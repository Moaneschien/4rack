#include "plugin.hpp"
#include "random.hpp"
#include "rndbezosccomponent.hpp"
using simd::float_4;

struct Rndbezosc : Module {
	enum ParamIds {
    PMORPH_PARAM,
		PFREQ_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		IFREQ_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
 


	Rndbezosc() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configParam(PMORPH_PARAM, 100, 5000, 2000, "Morp time");
		configParam(PFREQ_PARAM, -3.5f, 3.5f, 0.f, "Frequency", "Hz");
	}

  //random::init();
  inline std::array<simd::float_4, 4> genSpline(){
    std::array<simd::float_4, 4> BezierN;
    BezierN[0][0] = random::uniform();      // A  Knot
    BezierN[0][1] = random::uniform();      // Ab Handle
    BezierN[0][2] = random::uniform();      // Ba Handle
    BezierN[0][3] = random::uniform();      // B  Knot
    BezierN[0] = simd::rescale(BezierN[0], 0, 1,-2.5, 2.5);    // A  Knot
    //DEBUG("error: %f, %f, %f, %f",BezierN[0][0],BezierN[0][1],BezierN[0][2],BezierN[0][3]);
    BezierN[1][0] = BezierN[0][3];                                   // B  Knot
    BezierN[1][1] = BezierN[0][3] - (BezierN[0][2] - BezierN[0][3]); // Bc Handle
    BezierN[1][2] = rescale(random::uniform(), 0, 1,-2.5, 2.5);      // Cb Handle
    BezierN[1][3] = rescale(random::uniform(), 0, 1,-2.5, 2.5);      // C  Knot
    BezierN[2][0] = BezierN[1][3];                                   // C  Knot
    BezierN[2][1] = BezierN[1][3] - (BezierN[1][2] - BezierN[1][3]); // Cd Handle
    BezierN[2][2] = rescale(random::uniform(), 0, 1,-2.5, 2.5);      // Dc Handle
    BezierN[2][3] = rescale(random::uniform(), 0, 1,-2.5, 2.5);      // D  Knot
    BezierN[3][0] = BezierN[2][3];                                   // D  Knot
    BezierN[3][1] = BezierN[2][3] - (BezierN[2][2] - BezierN[2][3]); // Da Handle
    BezierN[3][2] = BezierN[0][0] - (BezierN[0][1] - BezierN[0][0]); // Ad Handle
    BezierN[3][3] = BezierN[0][0];                                   // A  Knot
    return BezierN;
  }

  std::array<simd::float_4, 4> bezierMorph = genSpline();
  std::array<simd::float_4, 4> bezierTarget;
  std::array<simd::float_4, 4> morph;

  bool morphFlag = false;
  int morphSteps;
  int morphStep = 0;
	float steps = 0.f;
	float pitchOld = 0.f;

	void process(const ProcessArgs& args) override {
		if(outputs[OUT_OUTPUT].isConnected()){

			float pitch = params[PFREQ_PARAM].getValue();
    	if(inputs[IFREQ_INPUT].isConnected()){
      	pitch += inputs[IFREQ_INPUT].getVoltage();
    	}
    	float freq = dsp::FREQ_C4 * powf(2.0f, pitch);
    	steps += args.sampleTime * freq*4;  // times 4 as there are 4 segments in the spline.
    	int arrIdx = floor(steps);
    	float t = steps - arrIdx;

      if (morphStep == 0){
        bezierTarget = genSpline();
        morphSteps = params[PMORPH_PARAM].getValue();
        for (int i = 0; i < 4; i++){
            morph[i] = (bezierTarget[i] - bezierMorph[i])/morphSteps;
        }
      }

    	if (arrIdx >= 4){
    	  arrIdx = 0;
    	  steps = t;
        morphFlag = true;
      } 

    	float t2 = t * t;
    	float t3 = t2 * t;
    	float tm = 1 - t;
    	float tm2 = tm * tm;
    	float tm3 = tm2 * tm;
      //B(t) = (1−t)^3P0 + 3(1−t)^2tP1 + 3(1−t)t^2P2 + t^3P3.
      float b1 = (bezierMorph[arrIdx][0])*tm3;
      float b2 = (bezierMorph[arrIdx][1])*3*tm2*t;
      float b3 = (bezierMorph[arrIdx][2])*3*tm*t2;
      float b4 = (bezierMorph[arrIdx][3])*t3;
      float bez = b1+b2+b3+b4;

      if(morphFlag){
        for (int i = 0; i < 4; i++){
            bezierMorph[i] += morph[i];
        }
      }
      if (morphStep++ >= morphSteps){
        morphStep = 0;
      }
			outputs[OUT_OUTPUT].setVoltage(bez);
    }
  }
};


struct RndbezoscWidget : ModuleWidget {
	RndbezoscWidget(Rndbezosc* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/rndbezosc.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addParam(createParamCentered<GreenKnob>(mm2px(Vec(15.037, 36.238)), module, Rndbezosc::PMORPH_PARAM));
		addParam(createParamCentered<HugeGreenKnob>(mm2px(Vec(12.693, 94.771)), module, Rndbezosc::PFREQ_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.698, 112.435)), module, Rndbezosc::IFREQ_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.078, 16.06)), module, Rndbezosc::OUT_OUTPUT));
	}
};


Model* modelRndbezosc = createModel<Rndbezosc, RndbezoscWidget>("rndbezosc");
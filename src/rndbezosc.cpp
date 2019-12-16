#include "plugin.hpp"
#include "random.hpp"
#include "rndbezosccomponent.hpp"
#include <array>

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

  bool morphFlag = false;
  int morphSteps;
  int morphStep = 0;
  float steps = 0.f;
  float pitchOld = 0.f;
  std::array<simd::float_4, 4> bezierMorph = genSpline();
  std::array<simd::float_4, 4> bezierTarget;
  std::array<simd::float_4, 4> morph;

  inline std::array<simd::float_4, 4> genSpline(){
    std::array<simd::float_4, 4> bezier;
    std::array<float, 8> rndp;
    for (int i = 0; i < 8; i++){rndp[i] = random::uniform();}
    bezier[0] = simd::rescale({rndp[0],rndp[1],rndp[2],rndp[3]}, 0, 1,-2.5, 2.5);
    simd::float_4 tmp = simd::rescale({rndp[4],rndp[5],rndp[6],rndp[7]}, 0, 1,-2.5, 2.5);
    bezier[1][0] = bezier[0][3];                                 // B  Knot
    bezier[1][1] = bezier[0][3] - (bezier[0][2] - bezier[0][3]); // Bc Handle
    bezier[1][2] = tmp[0];                                       // Cb Handle
    bezier[1][3] = tmp[1];                                       // C  Knot
    bezier[2][0] = bezier[1][3];                                 // C  Knot
    bezier[2][1] = bezier[1][3] - (bezier[1][2] - bezier[1][3]); // Cd Handle
    bezier[2][2] = tmp[3];                                       // Dc Handle
    bezier[2][3] = tmp[4];                                       // D  Knot
    bezier[3][0] = bezier[2][3];                                 // D  Knot
    bezier[3][1] = bezier[2][3] - (bezier[2][2] - bezier[2][3]); // Da Handle
    bezier[3][2] = bezier[0][0] - (bezier[0][1] - bezier[0][0]); // Ad Handle
    bezier[3][3] = bezier[0][0];                                 // A  Knot
    return bezier;
  }

  Rndbezosc() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configParam(PMORPH_PARAM, 100, 5000, 2000, "Morp steps");
    configParam(PFREQ_PARAM, -3.5f, 3.5f, 0.f, "Frequency", "Hz");
  }

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
#include "plugin.hpp"
#include "random.hpp"
#include "rndbezosccomponent.hpp"
#include <array>

using simd::float_4;

struct Rndbezosc : Module {
	enum ParamIds {
    PMORPH_PARAM,
		PFREQ_PARAM,
    STYLE_PARAM,
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

  static const int numSegments = 4;
  static const int pointsSegment = 4;
  static const int numPoints = numSegments * pointsSegment;


  int morphSteps = 0;
  int morphStep = 0;
  int tSteps = 0;
  float tStep = 0.f;
  float pitchOld = 0.f;
  std::array<simd::float_4, numSegments> bezierMorph = genSmoothSpline();
  std::array<simd::float_4, numSegments> bezierTarget;
  std::array<simd::float_4, numSegments> morph;

  inline std::array<simd::float_4, numSegments> genSmoothSpline(){
    std::array<simd::float_4, numSegments> bezier;
    std::array<float, 2*pointsSegment> rndp;
    for (int i = 0; i < 2*pointsSegment; i++){rndp[i] = random::uniform();}
    simd::float_4 tmp = simd::rescale({rndp[4],rndp[5],rndp[6],rndp[7]}, 0, 1,-2.5, 2.5);
    bezier[0] = simd::rescale({rndp[0],rndp[1],rndp[2],rndp[3]}, 0, 1,-2.5, 2.5);
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

  inline std::array<simd::float_4, numSegments> genWildSpline(){
    std::array<simd::float_4, numSegments> bezier;
    std::array<float, numPoints> rndp;
    for (int i = 0; i < numPoints; i++){rndp[i] = random::uniform();}
    bezier[0] = simd::rescale({rndp[0],rndp[1],rndp[2],rndp[3]}, 0, 1,-2.5, 2.5);
    bezier[1] = simd::rescale({rndp[4],rndp[5],rndp[6],rndp[7]}, 0, 1,-2.5, 2.5);
    bezier[2] = simd::rescale({rndp[8],rndp[9],rndp[10],rndp[11]}, 0, 1,-2.5, 2.5);
    bezier[3] = simd::rescale({rndp[12],rndp[13],rndp[14],rndp[15]}, 0, 1,-2.5, 2.5);
    return bezier;
  }

  inline std::array<simd::float_4, numSegments> genHalfWildSpline(){
    std::array<simd::float_4, numSegments> bezier;
    std::array<float, numPoints> rndp;
    for (int i = 0; i < 10; i++){rndp[i] = random::uniform();}
    bezier[0] = simd::rescale({rndp[0],rndp[1],rndp[2],rndp[3]}, 0, 1,-2.5, 2.5);
    bezier[1] = simd::rescale({rndp[4],rndp[5],rndp[6],rndp[7]}, 0, 1,-2.5, 2.5);
    bezier[2][0] = bezier[1][3];                                 // C  Knot
    bezier[2][1] = bezier[1][3] - (bezier[1][2] - bezier[1][3]); // Cd Handle
    bezier[2][2] = rescale(rndp[8], 0, 1,-2.5, 2.5);             // Dc Handle
    bezier[2][3] = rescale(rndp[8], 0, 1,-2.5, 2.5);             // D  Knot
    bezier[3][0] = bezier[2][3];                                 // D  Knot
    bezier[3][1] = bezier[2][3] - (bezier[2][2] - bezier[2][3]); // Da Handle
    bezier[3][2] = bezier[0][0] - (bezier[0][1] - bezier[0][0]); // Ad Handle
    bezier[3][3] = bezier[0][0];                                 // A  Knot
    return bezier;
  }

  Rndbezosc() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configParam(PMORPH_PARAM, 100, 5000, 2000, "Morph steps");
    configParam(PFREQ_PARAM, -3.5f, 3.5f, 0.f, "Frequency", "Hz");
    configParam(STYLE_PARAM, 0.f, 2.f, 0.f, "Modus");
  }

	void process(const ProcessArgs& args) override {
		if(outputs[OUT_OUTPUT].isConnected()){

      float pitch = rack::simd::ifelse(
         inputs[IFREQ_INPUT].isConnected()
        ,params[PFREQ_PARAM].getValue()+inputs[IFREQ_INPUT].getVoltage()
        ,params[PFREQ_PARAM].getValue()
      );

    	float freq = dsp::FREQ_C4 * powf(2.0f, pitch);
    	tStep += args.sampleTime * freq * numSegments;
    	int arrIdx = floor(tStep);
    	float t = tStep - arrIdx;

      if (morphStep == 0){
        int modus = params[STYLE_PARAM].getValue();
        if (modus == 0){bezierTarget = genSmoothSpline();}
        else if (modus == 1){bezierTarget = genHalfWildSpline();}
        else if (modus == 2){bezierTarget = genWildSpline();};        
        morphSteps = params[PMORPH_PARAM].getValue();
        for (int i = 0; i < 4; i++){
            morph[i] = (bezierTarget[i] - bezierMorph[i]) / morphSteps;
        }
      }

    	if (arrIdx >= numSegments){ // one complete cycle done
    	  arrIdx = 0;
    	  tStep = t;
        tSteps = 0;
      } 

    	float t2 = t * t;
    	float t3 = t2 * t;
    	float tm = 1 - t;
    	float tm2 = tm * tm;
    	float tm3 = tm2 * tm;
      float tm2t_3 = 3*tm2*t;
      float tmt2_3 = 3*tm*t2;
      float b1 = (bezierMorph[arrIdx][0])*tm3;
      float b2 = (bezierMorph[arrIdx][1])*tm2t_3;
      float b3 = (bezierMorph[arrIdx][2])*tmt2_3;
      float b4 = (bezierMorph[arrIdx][3])*t3;
      float bez = b1+b2+b3+b4;

      for (int i = 0; i < numSegments; i++){
          bezierMorph[i] += morph[i];
      }

      if (morphStep++ >= morphSteps){
        morphStep = 0;
      }
      tSteps++;
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
    addParam(createParamCentered<ModusThree>(mm2px(Vec(20.473, 66.338)), module, Rndbezosc::STYLE_PARAM));
		addParam(createParamCentered<HugeGreenKnob>(mm2px(Vec(12.693, 94.771)), module, Rndbezosc::PFREQ_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.698, 112.435)), module, Rndbezosc::IFREQ_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.078, 16.06)), module, Rndbezosc::OUT_OUTPUT));
	}
};

Model* modelRndbezosc = createModel<Rndbezosc, RndbezoscWidget>("rndbezosc");
#include "plugin.hpp"


#include <cmath>

struct Ramp : Module {
	enum ParamIds {
		ENUMS(VFROM_PARAMS, 8),
		ENUMS(VTO_PARAMS, 8),
		ENUMS(TIME_PARAMS, 8),
		ENUMS(INTERP_PARAMS, 8),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(START_INPUT, 8),
		ENUMS(STOP_INPUT, 8),
		NUM_INPUT
	};
	enum OutputIds {
		ENUMS(END_OUTPUT, 8),
		ENUMS(VOUT_OUTPUT, 8),
		NUM_OUTPUT
	};
	enum LightIds {
		NUM_LIGHTS
	};


	/** Interpolates over gs, ge and rescales to ts, te.
	gc: global current
	//gs: global start omitted as gs is always 0
	ge: global end
	ts: target start
	te: target end
	im: interpolation method,
	    im = 0     - cosine interpolation
	    0> im <1   - exponential (ease in)
	    im = 1     - linear
	    1> im <10  - exponential (ease out)
	    im = 10    - step
	*/
	inline float interpolate(float gc, float ge, float ts, float te, float im) {
	  float v;
	  if (im == 10.f) {(gc == ge) ? v = te : v = ts;}  // step
	  else {
	    float x = gc / ge;
	    if (im != 0) { 
	      (im == 1.f) ? v = ts + (te - ts) * x : v = ts + (te - ts) * pow(x, im);
	    }
	    else {                             				// cosine
	      float f = (1 - cos(x * M_PI)) * 0.5;
	      v = ts * (1 - f) + te * f;
	    }  
	  }
	  return v;
	}

	float gc[8];
	bool running[8];
	bool finished[8];
	bool stopped[8];
	bool endPulse[8];
	dsp::SchmittTrigger startTrigger[8];
	dsp::SchmittTrigger stopTrigger[8];
	dsp::PulseGenerator endPulseGen[8];

	Ramp() {
		config(NUM_PARAMS, NUM_INPUT, NUM_OUTPUT, NUM_LIGHTS);
		for (int i = 0; i < 8; i++) {
			configParam(VFROM_PARAMS + i, 0.f, 10.f, 0.f, string::f("Voltage %d from", i + 1));
			configParam(VTO_PARAMS + i, 0.f, 10.f, 0.f, string::f("Voltage %d to", i + 1));
			configParam(TIME_PARAMS + i, 0.f, 1200.f, 0.f, string::f("time %d", i + 1), "s");
			configParam(INTERP_PARAMS + i, 0.f, 10.f, 0.f, string::f("interpolate %d", i + 1));
			gc[i] = 0;
			running[i] = false;
			finished[i] = false;
			stopped[i] = true;
			endPulse[i] = false;
		}
	}

	void process(const ProcessArgs& args) override {
		for (int i = 0; i < 8; i++) {
			if (inputs[START_INPUT + i].isConnected() && outputs[VOUT_OUTPUT + i].isConnected() == true) {
				if (running[i] == true) {
					gc[i] += args.sampleTime;				
					if (gc[i] < params[TIME_PARAMS + i].getValue()) {
						outputs[VOUT_OUTPUT + i].setVoltage(
							interpolate(
								gc[i], 
								params[TIME_PARAMS + i].getValue(), 
								params[VFROM_PARAMS + i].getValue(), 
								params[VTO_PARAMS + i].getValue(), 
								params[INTERP_PARAMS + i].getValue()
							)
						);
					}
					else {
						running[i] = false;
						finished[i] = true;
						stopped[i] = false;
						endPulseGen[i].trigger(1e-3f);
					}
				}
				else if (finished[i] == true) {
					endPulse[i] = endPulseGen[i].process(1/args.sampleTime);
					outputs[END_OUTPUT + i].setVoltage(endPulse[i] ? 10.f : 0.f);
					outputs[VOUT_OUTPUT + i].setVoltage(params[VTO_PARAMS + i].getValue());
				}
				if (startTrigger[i].process(inputs[START_INPUT + i].getVoltage()) == true) {
					gc[i] = 0;
					running[i] = true;
					finished[i] = false;
					stopped[i] = false;
				}
				if (inputs[STOP_INPUT + i].isConnected() && stopTrigger[i].process(inputs[STOP_INPUT + i].getVoltage()) == true) {
					gc[i] = 0;
					running[i] = false;
					finished[i] = false;
					stopped[i] = true;
					outputs[VOUT_OUTPUT + i].setVoltage(0.f);
				}
			}
			else {
				outputs[VOUT_OUTPUT + i].setVoltage(0.f);
				gc[i] = 0;
			}
		}
	}
};

struct RampWidget : ModuleWidget {
	RampWidget(Ramp* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Ramp.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(38.1, 18.603)), module, Ramp::VFROM_PARAMS + 0));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(50.8, 18.603)), module, Ramp::VTO_PARAMS + 0));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(63.5, 18.603)), module, Ramp::TIME_PARAMS + 0));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(76.2, 18.603)), module, Ramp::INTERP_PARAMS + 0));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(38.1, 31.453)), module, Ramp::VFROM_PARAMS + 1));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(50.8, 31.453)), module, Ramp::VTO_PARAMS + 1));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(63.5, 31.453)), module, Ramp::TIME_PARAMS + 1));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(76.2, 31.453)), module, Ramp::INTERP_PARAMS + 1));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(38.1, 44.303)), module, Ramp::VFROM_PARAMS + 2));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(50.8, 44.303)), module, Ramp::VTO_PARAMS + 2));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(63.5, 44.303)), module, Ramp::TIME_PARAMS + 2));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(76.2, 44.303)), module, Ramp::INTERP_PARAMS + 2));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(38.1, 57.153)), module, Ramp::VFROM_PARAMS + 3));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(50.8, 57.153)), module, Ramp::VTO_PARAMS + 3));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(63.5, 57.153)), module, Ramp::TIME_PARAMS + 3));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(76.2, 57.153)), module, Ramp::INTERP_PARAMS + 3));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(38.1, 70.003)), module, Ramp::VFROM_PARAMS + 4));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(50.8, 70.003)), module, Ramp::VTO_PARAMS + 4));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(63.5, 70.003)), module, Ramp::TIME_PARAMS + 4));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(76.2, 70.003)), module, Ramp::INTERP_PARAMS + 4));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(38.1, 82.853)), module, Ramp::VFROM_PARAMS + 5));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(50.8, 82.853)), module, Ramp::VTO_PARAMS + 5));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(63.5, 82.853)), module, Ramp::TIME_PARAMS + 5));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(76.2, 82.853)), module, Ramp::INTERP_PARAMS + 5));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(38.1, 95.703)), module, Ramp::VFROM_PARAMS + 6));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(50.8, 95.703)), module, Ramp::VTO_PARAMS + 6));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(63.5, 95.703)), module, Ramp::TIME_PARAMS + 6));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(76.2, 95.703)), module, Ramp::INTERP_PARAMS + 6));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(38.1, 108.553)), module, Ramp::VFROM_PARAMS + 7));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(50.8, 108.553)), module, Ramp::VTO_PARAMS + 7));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(63.5, 108.553)), module, Ramp::TIME_PARAMS + 7));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(76.2, 108.553)), module, Ramp::INTERP_PARAMS + 7));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 18.603)), module, Ramp::START_INPUT + 0));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.4, 18.603)), module, Ramp::STOP_INPUT + 0));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 31.453)), module, Ramp::START_INPUT + 1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.4, 31.453)), module, Ramp::STOP_INPUT + 1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 44.303)), module, Ramp::START_INPUT + 2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.4, 44.303)), module, Ramp::STOP_INPUT + 2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 57.153)), module, Ramp::START_INPUT + 3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.4, 57.153)), module, Ramp::STOP_INPUT + 3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 70.003)), module, Ramp::START_INPUT + 4));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.4, 70.003)), module, Ramp::STOP_INPUT + 4));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 82.853)), module, Ramp::START_INPUT + 5));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.4, 82.853)), module, Ramp::STOP_INPUT + 5));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 95.703)), module, Ramp::START_INPUT + 6));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.4, 95.703)), module, Ramp::STOP_INPUT + 6));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 108.553)), module, Ramp::START_INPUT + 7));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.4, 108.553)), module, Ramp::STOP_INPUT + 7));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.9, 18.603)), module, Ramp::END_OUTPUT + 0));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(101.6, 18.603)), module, Ramp::VOUT_OUTPUT + 0));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.9, 31.453)), module, Ramp::END_OUTPUT + 1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(101.6, 31.453)), module, Ramp::VOUT_OUTPUT + 1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.9, 44.303)), module, Ramp::END_OUTPUT + 2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(101.6, 44.303)), module, Ramp::VOUT_OUTPUT + 2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.9, 57.153)), module, Ramp::END_OUTPUT + 3));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(101.6, 57.153)), module, Ramp::VOUT_OUTPUT + 3));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.9, 70.003)), module, Ramp::END_OUTPUT + 4));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(101.6, 70.003)), module, Ramp::VOUT_OUTPUT + 4));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.9, 82.853)), module, Ramp::END_OUTPUT + 5));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(101.6, 82.853)), module, Ramp::VOUT_OUTPUT + 5));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.9, 95.703)), module, Ramp::END_OUTPUT + 6));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(101.6, 95.703)), module, Ramp::VOUT_OUTPUT + 6));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.9, 108.553)), module, Ramp::END_OUTPUT + 7));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(101.6, 108.553)), module, Ramp::VOUT_OUTPUT + 7));
	}
};


Model* modelRamp = createModel<Ramp, RampWidget>("Ramp");
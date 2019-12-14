#include "plugin.hpp"
#include "rampcomponent.hpp"
#include <cmath>
using simd::float_4;

struct Ramp : Module {
	enum ParamIds {
		ENUMS(VFROM_PARAM, 8),
		ENUMS(VTO_PARAM, 8),
		ENUMS(TIME_PARAM, 8),
		ENUMS(INTERP_PARAM, 8),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(START_INPUT, 8),
		ENUMS(STOP_INPUT, 8),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(END_OUTPUT, 8),
		ENUMS(VOUTB_OUTPUT, 8),
		ENUMS(VOUTU_OUTPUT, 8),
		NUM_OUTPUT
	};
	enum LightIds {
		ENUMS(START_LIGHT, 8),
		ENUMS(END_LIGHT, 8),
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
		if (im == 0.f) {                             				// cosine
	      float f = (1 - simd::cos((gc / ge) * M_PI)) * 0.5;
	      v = ts * (1 - f) + te * f;
	  }
	  else if (im == 1.f) {v = ts + (te - ts) * (gc / ge);}
	  else if (im == 10.f) {(gc == ge) ? v = te : v = ts;}  // step
	  else {v = ts + (te - ts) * simd::pow((gc / ge), im);}
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
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUT, NUM_LIGHTS);
		for (int i = 0; i < 8; i++) {
			configParam(VFROM_PARAM + i, 0.f, 10.f, 0.f, string::f("Voltage %d from", i + 1));
			configParam(VTO_PARAM + i, 0.f, 10.f, 0.f, string::f("Voltage %d to", i + 1));
			configParam(TIME_PARAM + i, 0.f, 1200.f, 0.f, string::f("time %d", i + 1), "s");
			configParam(INTERP_PARAM + i, 0.f, 10.f, 0.f, string::f("interpolate %d", i + 1));
			gc[i] = 0;
			running[i] = false;
			finished[i] = false;
			stopped[i] = true;
			endPulse[i] = false;
		}
		onReset();
	}

	void process(const ProcessArgs& args) override {
		for (int i = 0; i < 8; i++) {
			if (
				inputs[START_INPUT + i].isConnected() 
				&& (
					outputs[END_OUTPUT + i].isConnected()
					|| outputs[VOUTU_OUTPUT + i].isConnected() 
					|| outputs[VOUTB_OUTPUT + i].isConnected()
				) == true
			) {
				if (startTrigger[i].process(inputs[START_INPUT + i].getVoltage()) == true) {
					gc[i] = 0;
					running[i] = true;
					finished[i] = false;
					stopped[i] = false;
				}
				if (running[i] == true) {
					gc[i] += args.sampleTime;				
					if (gc[i] < params[TIME_PARAM + i].getValue()) {
						float current_voltage = interpolate(
							gc[i], 
							params[TIME_PARAM + i].getValue(), 
							params[VFROM_PARAM + i].getValue(), 
							params[VTO_PARAM + i].getValue(), 
							params[INTERP_PARAM + i].getValue()
						);		
						outputs[VOUTU_OUTPUT + i].setVoltage(current_voltage);
						outputs[VOUTB_OUTPUT + i].setVoltage(
							rescale(current_voltage, 0.f, 10.f, -5.f, 5.f)
						);
					}
					else {
						running[i] = false;
						finished[i] = true;
						stopped[i] = false;
						endPulseGen[i].trigger(1e-3f);
					}
					lights[START_LIGHT + i].setBrightness(10.f);
				}
				else if (finished[i] == true) {
					endPulse[i] = endPulseGen[i].process(args.sampleTime);
					outputs[END_OUTPUT + i].setVoltage(endPulse[i] ? 10.f : 0.f);
					float current_voltage = params[VTO_PARAM + i].getValue();
					outputs[VOUTU_OUTPUT + i].setVoltage(current_voltage);
					outputs[VOUTB_OUTPUT + i].setVoltage(
						rescale(current_voltage, 0.0 ,10.0, -5.0, 5.0)
					);
					lights[START_LIGHT + i].setBrightness(10.f);
					lights[END_LIGHT + i].setBrightness(10.f);
				}
				if (inputs[STOP_INPUT + i].isConnected() && stopTrigger[i].process(inputs[STOP_INPUT + i].getVoltage()) == true) {
					gc[i] = 0;
					running[i] = false;
					finished[i] = false;
					stopped[i] = true;
					outputs[END_OUTPUT + i].setVoltage(0.f);
					outputs[VOUTU_OUTPUT + i].setVoltage(0.f);
					outputs[VOUTB_OUTPUT + i].setVoltage(0.f);
					lights[END_LIGHT + i].setBrightness(0.f);
					lights[START_LIGHT + i].setBrightness(0.f);
				}
			}
			else {
				outputs[VOUTU_OUTPUT + i].setVoltage(0.f);
				outputs[VOUTB_OUTPUT + i].setVoltage(0.f);
				lights[END_LIGHT + i].setBrightness(0.f);
				lights[START_LIGHT + i].setBrightness(0.f);
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

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.0,  34.5)), module, Ramp::START_INPUT + 0));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.0,  46.5)), module, Ramp::START_INPUT + 1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.0,  58.5)), module, Ramp::START_INPUT + 2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.0,  70.5)), module, Ramp::START_INPUT + 3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.0,  82.5)), module, Ramp::START_INPUT + 4));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.0,  94.5)), module, Ramp::START_INPUT + 5));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.0, 106.5)), module, Ramp::START_INPUT + 6));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.0, 118.5)), module, Ramp::START_INPUT + 7));

		addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(11.526,  38.044)), module, Ramp::START_LIGHT + 0));
		addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(11.526,  50.044)), module, Ramp::START_LIGHT + 1));
		addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(11.526,  62.044)), module, Ramp::START_LIGHT + 2));
		addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(11.526,  74.044)), module, Ramp::START_LIGHT + 3));
		addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(11.526,  86.044)), module, Ramp::START_LIGHT + 4));
		addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(11.526,  98.044)), module, Ramp::START_LIGHT + 5));
		addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(11.526, 110.044)), module, Ramp::START_LIGHT + 6));
		addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(11.526, 122.044)), module, Ramp::START_LIGHT + 7));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.0,  34.5)), module, Ramp::STOP_INPUT + 0));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.0,  46.5)), module, Ramp::STOP_INPUT + 1));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.0,  58.5)), module, Ramp::STOP_INPUT + 2));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.0,  70.5)), module, Ramp::STOP_INPUT + 3));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.0,  82.5)), module, Ramp::STOP_INPUT + 4));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.0,  94.5)), module, Ramp::STOP_INPUT + 5));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.0, 106.5)), module, Ramp::STOP_INPUT + 6));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.0, 118.5)), module, Ramp::STOP_INPUT + 7));

		addParam(createParamCentered<HoleKnob>(mm2px(Vec(30.0,  34.5)), module, Ramp::VFROM_PARAM + 0));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(30.0,  46.5)), module, Ramp::VFROM_PARAM + 1));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(30.0,  58.5)), module, Ramp::VFROM_PARAM + 2));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(30.0,  70.5)), module, Ramp::VFROM_PARAM + 3));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(30.0,  82.5)), module, Ramp::VFROM_PARAM + 4));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(30.0,  94.5)), module, Ramp::VFROM_PARAM + 5));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(30.0, 106.5)), module, Ramp::VFROM_PARAM + 6));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(30.0, 118.5)), module, Ramp::VFROM_PARAM + 7));

		addParam(createParamCentered<HoleKnob>(mm2px(Vec(42.0,  34.5)), module, Ramp::VTO_PARAM + 0));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(42.0,  46.5)), module, Ramp::VTO_PARAM + 1));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(42.0,  58.5)), module, Ramp::VTO_PARAM + 2));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(42.0,  70.5)), module, Ramp::VTO_PARAM + 3));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(42.0,  82.5)), module, Ramp::VTO_PARAM + 4));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(42.0,  94.5)), module, Ramp::VTO_PARAM + 5));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(42.0, 106.5)), module, Ramp::VTO_PARAM + 6));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(42.0, 118.5)), module, Ramp::VTO_PARAM + 7));

		addParam(createParamCentered<HoleKnob>(mm2px(Vec(54.0,  34.5)), module, Ramp::TIME_PARAM + 0));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(54.0,  46.5)), module, Ramp::TIME_PARAM + 1));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(54.0,  58.5)), module, Ramp::TIME_PARAM + 2));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(54.0,  70.5)), module, Ramp::TIME_PARAM + 3));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(54.0,  82.5)), module, Ramp::TIME_PARAM + 4));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(54.0,  94.5)), module, Ramp::TIME_PARAM + 5));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(54.0, 106.5)), module, Ramp::TIME_PARAM + 6));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(54.0, 118.5)), module, Ramp::TIME_PARAM + 7));

		addParam(createParamCentered<HoleKnob>(mm2px(Vec(66.0,  34.5)), module, Ramp::INTERP_PARAM + 0));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(66.0,  46.5)), module, Ramp::INTERP_PARAM + 1));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(66.0,  58.5)), module, Ramp::INTERP_PARAM + 2));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(66.0,  70.5)), module, Ramp::INTERP_PARAM + 3));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(66.0,  82.5)), module, Ramp::INTERP_PARAM + 4));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(66.0,  94.5)), module, Ramp::INTERP_PARAM + 5));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(66.0, 106.5)), module, Ramp::INTERP_PARAM + 6));
		addParam(createParamCentered<HoleKnob>(mm2px(Vec(66.0, 118.5)), module, Ramp::INTERP_PARAM + 7));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.0, 	34.5)), module, Ramp::END_OUTPUT + 0));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.0, 	46.5)), module, Ramp::END_OUTPUT + 1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.0, 	58.5)), module, Ramp::END_OUTPUT + 2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.0, 	70.5)), module, Ramp::END_OUTPUT + 3));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.0, 	82.5)), module, Ramp::END_OUTPUT + 4));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.0, 	94.5)), module, Ramp::END_OUTPUT + 5));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.0, 106.5)), module, Ramp::END_OUTPUT + 6));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78.0, 118.5)), module, Ramp::END_OUTPUT + 7));

		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(81.526,  38.044)), module, Ramp::END_LIGHT + 0));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(81.526,  50.044)), module, Ramp::END_LIGHT + 1));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(81.526,  62.044)), module, Ramp::END_LIGHT + 2));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(81.526,  74.044)), module, Ramp::END_LIGHT + 3));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(81.526,  86.044)), module, Ramp::END_LIGHT + 4));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(81.526,  98.044)), module, Ramp::END_LIGHT + 5));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(81.526, 110.044)), module, Ramp::END_LIGHT + 6));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(81.526, 121.898)), module, Ramp::END_LIGHT + 7));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.0,  34.5)), module, Ramp::VOUTB_OUTPUT + 0));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.0,  46.5)), module, Ramp::VOUTB_OUTPUT + 1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.0,  58.5)), module, Ramp::VOUTB_OUTPUT + 2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.0,  70.5)), module, Ramp::VOUTB_OUTPUT + 3));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.0,  82.5)), module, Ramp::VOUTB_OUTPUT + 4));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.0,  94.5)), module, Ramp::VOUTB_OUTPUT + 5));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.0, 106.5)), module, Ramp::VOUTB_OUTPUT + 6));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.0, 118.5)), module, Ramp::VOUTB_OUTPUT + 7));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(98.0,  34.5)), module, Ramp::VOUTU_OUTPUT + 0));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(98.0,  46.5)), module, Ramp::VOUTU_OUTPUT + 1));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(98.0,  58.5)), module, Ramp::VOUTU_OUTPUT + 2));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(98.0,  70.5)), module, Ramp::VOUTU_OUTPUT + 3));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(98.0,  82.5)), module, Ramp::VOUTU_OUTPUT + 4));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(98.0,  94.5)), module, Ramp::VOUTU_OUTPUT + 5));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(98.0, 106.5)), module, Ramp::VOUTU_OUTPUT + 6));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(98.0, 118.5)), module, Ramp::VOUTU_OUTPUT + 7));
	}
};

Model* modelRamp = createModel<Ramp, RampWidget>("Ramp");
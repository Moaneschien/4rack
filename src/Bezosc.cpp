#include "plugin.hpp"
#include "bezosccomponent.hpp"
using simd::float_4;

struct Bezosc : Module {
	enum ParamIds {
		ENUMS(PBEZ_PARAM, 24),
		PBEZSCALEX_PARAM,
		PBEZSCALEY_PARAM,
		PBEZSCALETH_PARAM,
		PBEZSCALEL_PARAM,
		PTANSCALEX_PARAM,
		PTANSCALEY_PARAM,
		PTANSCALETH_PARAM,
		PTANSCALEL_PARAM,
		PBEZFREQ_PARAM,
    MODUS_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(IBEZ_INPUT, 24),
		IBEZFREQ_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OBEZX_OUTPUT,
		OBEZY_OUTPUT,
		OBEZTH_OUTPUT,
		OBEZL_OUTPUT,
		OTANX_OUTPUT,
		OTANY_OUTPUT,
		OTANTH_OUTPUT,
		OTANL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
    ENUMS(LLED_LIGHT, 28),
		NUM_LIGHTS
	};

  static const int numSegments = 4;
  static const int pointsSegment = 4;
  static const int numDims = 2;
  static const int doublePoints = 4;
  static const int numXY = (numSegments * pointsSegment * numDims) - (doublePoints * numDims);

  const float defaults[numXY] = { //approx. circle r=4
    -2.2092f, 4.f,     0.f, 4.f, 2.2092f, 4.f,
     4.f,     2.2092f, 4.f, 0.f, 4.f,    -2.2092f,
     2.2092f,-4.f,     0.f,-4.f,-2.2092f,-4.f,
    -4.f,    -2.2092f,-4.f, 0.f,-4.f,     2.2092f
  };
  const float theLeds[numSegments][NUM_LIGHTS] = {
    {
      10.f,10.f,10.f,10.f, 0.f,10.f,10.f,
      10.f,10.f,10.f,10.f, 0.f,10.f,10.f,
      10.f,10.f,10.f,10.f, 0.f,10.f,10.f,
      10.f,10.f,10.f,10.f, 0.f,10.f,10.f
    },{
      10.f,10.f,10.f,10.f,10.f,10.f,10.f,
      10.f,10.f,10.f,10.f,10.f,10.f,10.f,
      10.f,10.f,10.f,10.f,10.f,10.f,10.f,
      10.f,10.f,10.f,10.f,10.f,10.f,10.f
    },{
      10.f, 0.f,10.f,10.f,10.f,10.f,10.f,
      10.f, 0.f,10.f,10.f,10.f,10.f,10.f,
       0.f,10.f,10.f,10.f,10.f,10.f,10.f,
       0.f,10.f,10.f,10.f,10.f,10.f,10.f
    },{
      0.f, 0.f,10.f,10.f,10.f,10.f,10.f,
      0.f, 0.f,10.f,10.f,10.f,10.f,10.f,
      0.f, 0.f,10.f,10.f,10.f,10.f,10.f,
      0.f, 0.f,10.f,10.f,10.f,10.f,10.f
    }
  };

  float steps = 0;
  float oldModus = 0;

	Bezosc() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    for (int i = 0; i < numXY; i++) {
		  configParam(PBEZ_PARAM + i, -10.f, 10.f, defaults[i], "Val");
    }
		configParam(PBEZSCALEX_PARAM,  0.f, 2.f, 1.f, "bezier x");
		configParam(PBEZSCALEY_PARAM,  0.f, 2.f, 1.f, "bezier y");
		configParam(PBEZSCALETH_PARAM, 0.f, 2.f, 1.f, "bezier theta");
		configParam(PBEZSCALEL_PARAM,  0.f, 2.f, 1.f, "bezier len");
		configParam(PTANSCALEX_PARAM,  0.f, 2.f, 0.604f, "tangent x");
		configParam(PTANSCALEY_PARAM,  0.f, 2.f, 0.604f, "tangent y");
		configParam(PTANSCALETH_PARAM, 0.f, 2.f, 1.f, "tangent theta");
		configParam(PTANSCALEL_PARAM,  0.f, 2.f, 0.604f, "tangent len");
		configParam(PBEZFREQ_PARAM,   -3.f, 3.f, 0.f, "frequency");
    configParam(MODUS_PARAM,       1.f, 4.f, 1.f, "modus");
	}
 
	void process(const ProcessArgs& args) override {
    bool obez = (  
         outputs[OBEZX_OUTPUT].isConnected()  || outputs[OBEZY_OUTPUT].isConnected()
      || outputs[OBEZTH_OUTPUT].isConnected() || outputs[OBEZL_OUTPUT].isConnected()
    );
    bool otan = (  
         outputs[OTANX_OUTPUT].isConnected()  || outputs[OTANY_OUTPUT].isConnected()
      || outputs[OTANTH_OUTPUT].isConnected() || outputs[OTANL_OUTPUT].isConnected()
    );
    if (obez || otan){
      int modus = params[MODUS_PARAM].getValue();
      if(modus != oldModus){
        for (int i = 0; i < NUM_LIGHTS; i++){
          lights[LLED_LIGHT + i].setBrightness(theLeds[modus-1][i]);
        }
        oldModus = modus;
      }
      // get all spline input values and arrange it into four segements, 
      // accounting for the current mode.
      float xy[numXY];
      for (int i = 0; i < numXY; i++){
        xy[i] = rack::simd::ifelse(
           inputs[IBEZ_INPUT + i].isConnected()
          ,params[PBEZ_PARAM + i].getValue() + inputs[IBEZ_INPUT + i].getVoltage()
          ,params[PBEZ_PARAM + i].getValue()
        );
      }
      Vec Ad, Ab, Ba, Bc, Cb, Cd, Dc, Da;
      Vec A = Vec(xy[2], xy[3]);
      Vec B = Vec(xy[8], xy[9]);
      Vec C = Vec(xy[14],xy[15]);
      Vec D = Vec(xy[20],xy[21]);
      if(modus == 1){
        // Independent rough mode.
        Ad = Vec(xy[0], xy[1]);
        Ab = Vec(xy[4], xy[5]);
        Ba = Vec(xy[6], xy[7]);
        Bc = Vec(xy[10],xy[11]);
        Cb = Vec(xy[12],xy[13]);
        Cd = Vec(xy[16],xy[17]);
        Dc = Vec(xy[18],xy[19]);
        Da = Vec(xy[22],xy[23]);
      }
      else if(modus == 2){
        // Dependent rough mode, handles move along with knots. 
        // Handles can be set independent.
        Ab = Vec(xy[4], xy[5]).plus(A);
        Ba = Vec(xy[6], xy[7]).plus(B);
        Bc = Vec(xy[10],xy[11]).plus(B);
        Cb = Vec(xy[12],xy[13]).plus(C);
        Cd = Vec(xy[16],xy[17]).plus(C);
        Dc = Vec(xy[18],xy[19]).plus(D);
        Da = Vec(xy[22],xy[23]).plus(D);
        Ad = Vec(xy[0], xy[1]).plus(A);
      }
      else if(modus == 3){    
        // Dependent semi smooth, handles move along with knots. One handle can be set independent. 
        // The other is only variable in length, positive as well as negative.
        Ab = A.plus(Vec(xy[4], xy[5]));
        Bc = B.plus(Vec(xy[10],xy[11]));
        Cd = C.plus(Vec(xy[16],xy[17]));
        Da = D.plus(Vec(xy[22],xy[23]));
        Ba = B.minus((Vec(xy[10],xy[11]).normalize()).mult(xy[6]));
        Cb = C.minus(Vec(xy[16],xy[17]).normalize()).mult(xy[12]);
        Dc = D.minus(Vec(xy[22],xy[23]).normalize()).mult(xy[18]);
        Ad = A.minus(Vec(xy[4], xy[5]).normalize()).mult(xy[0]);
        params[PBEZ_PARAM +  7].setValue(xy[6]);
        params[PBEZ_PARAM + 13].setValue(xy[12]);
        params[PBEZ_PARAM + 19].setValue(xy[18]);
        params[PBEZ_PARAM +  1].setValue(xy[0]);
      }
      else if(modus == 4){
      // Smooth, handles move along with knots. One handle can be set independent, 
      // the other is equal in opposite direction.
        Ab = A.plus(Vec(xy[4], xy[5]));
        Bc = B.plus(Vec(xy[10],xy[11]));
        Cd = C.plus(Vec(xy[16],xy[17]));
        Da = D.plus(Vec(xy[22],xy[23]));
        Ba = B.minus(Vec(xy[10],xy[11]));
        Cb = C.minus(Vec(xy[16],xy[17]));
        Dc = D.minus(Vec(xy[22],xy[23]));
        Ad = A.minus(Vec(xy[4], xy[5]));
        params[PBEZ_PARAM +  6].setValue(-xy[10]);
        params[PBEZ_PARAM +  7].setValue(-xy[11]);
        params[PBEZ_PARAM + 12].setValue(-xy[16]);
        params[PBEZ_PARAM + 13].setValue(-xy[17]);
        params[PBEZ_PARAM + 18].setValue(-xy[22]);
        params[PBEZ_PARAM + 19].setValue(-xy[23]);
        params[PBEZ_PARAM +  0].setValue(-xy[4]);
        params[PBEZ_PARAM +  1].setValue(-xy[5]);
      };
      Vec bezier[numSegments][pointsSegment] = {
        {A, Ab, Ba, B},
        {B, Bc, Cb, C},
        {C, Cd, Dc, D},
        {D, Da, Ad, A}
      };
      
      float pitch = rack::simd::ifelse(
         inputs[IBEZFREQ_INPUT].isConnected()
        ,params[PBEZFREQ_PARAM].getValue() + inputs[IBEZFREQ_INPUT].getVoltage()
        ,params[PBEZFREQ_PARAM].getValue()
      );

      float freq = dsp::FREQ_C4 * powf(2.0f, pitch);
      steps += args.sampleTime * freq * numSegments;  // times 4 as there are 4 segments in the spline.
      int arrIdx = floor(steps);
      float t = steps - arrIdx;

      if (arrIdx >= numSegments){
        arrIdx = 0;
        steps = t;
      }

      float t2 = t * t;
      float t3 = t2 * t;
      float tm = 1 - t;
      float tm2 = tm * tm;
      float tm3 = tm2 * tm;

      if(obez){
        //Position vector @ t, on bezier section.
        //B(t) = (1−t)^3P0 + 3(1−t)^2tP1 + 3(1−t)t^2P2 + t^3P3.
        Vec b1 = bezier[arrIdx][0].mult(tm3);
        Vec b2 = bezier[arrIdx][1].mult(3*tm2*t);
        Vec b3 = bezier[arrIdx][2].mult(3*tm*t2);
        Vec b4 = bezier[arrIdx][3].mult(t3);
        Vec bez = b1.plus(b2).plus(b3).plus(b4);
        if(outputs[OBEZX_OUTPUT].isConnected()){
          outputs[OBEZX_OUTPUT].setVoltage(bez.x * params[PBEZSCALEX_PARAM].getValue());
        }
        if(outputs[OBEZY_OUTPUT].isConnected()){
          outputs[OBEZY_OUTPUT].setVoltage(bez.y * params[PBEZSCALEY_PARAM].getValue());
        }
        if(outputs[OBEZTH_OUTPUT].isConnected()){ //angle vector (x,y).
          outputs[OBEZTH_OUTPUT].setVoltage(atan2(bez.y, bez.x) * params[PBEZSCALETH_PARAM].getValue());
        }
        if(outputs[OBEZL_OUTPUT].isConnected()){  //length (x,y).
          float v = sqrt(pow(bez.x,2)+pow(bez.y,2)) * params[PBEZSCALEL_PARAM].getValue();
          if(sgn(bez.y) == -1){v = -v;}
          outputs[OBEZL_OUTPUT].setVoltage(v);
        }
      }
      if(otan){
        //Tangent vector @ t, on first derivative of bezier.
        //B′(t)= 3(1−t)^2(P1−P0) + 6(1−t)t(P2−P1) + 3t^2(P3−P2).
        Vec c1 = (bezier[arrIdx][1].minus(bezier[arrIdx][0])).mult(3 * tm2);
        Vec c2 = (bezier[arrIdx][2].minus(bezier[arrIdx][1])).mult(6 * tm * t);
        Vec c3 = (bezier[arrIdx][3].minus(bezier[arrIdx][2])).mult(3 * t2);
        Vec beztan = c1.plus(c2).plus(c3);
        if(outputs[OTANX_OUTPUT].isConnected()){
          outputs[OTANX_OUTPUT].setVoltage(beztan.x * params[PTANSCALEX_PARAM].getValue());
        }
        if(outputs[OTANY_OUTPUT].isConnected()){
          outputs[OTANY_OUTPUT].setVoltage(beztan.y * params[PTANSCALEY_PARAM].getValue());
        }
        if(outputs[OTANTH_OUTPUT].isConnected()){ //angle of tangent vector.
          outputs[OTANTH_OUTPUT].setVoltage(atan2(beztan.y, beztan.x) * params[PTANSCALETH_PARAM].getValue());
        }
        if(outputs[OTANL_OUTPUT].isConnected()){ //length of tangent vector.
          float v = sqrt(pow(beztan.x,2)+pow(beztan.y,2)) * params[PTANSCALEL_PARAM].getValue();
          if(sgn(beztan.y) == -1){v = -v;}
          outputs[OTANL_OUTPUT].setVoltage(v);
        }
      }
    }
  }
};


struct BezoscWidget : ModuleWidget {
	BezoscWidget(Bezosc* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Bezosc.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 25.934,  16.068)), module, Bezosc::PBEZ_PARAM +  0));
		addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 39.983,  16.068)), module, Bezosc::PBEZ_PARAM +  1));
		addParam(createParamCentered<LargeCyanHoleKnob>(mm2px(Vec( 53.726,  16.068)), module, Bezosc::PBEZ_PARAM +  2));
		addParam(createParamCentered<LargeCyanHoleKnob>(mm2px(Vec( 67.907,  16.068)), module, Bezosc::PBEZ_PARAM +  3));
		addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 81.810,  16.068)), module, Bezosc::PBEZ_PARAM +  4));
		addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 95.847,  16.068)), module, Bezosc::PBEZ_PARAM +  5));
 
    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec(108.017,  29.299)), module, Bezosc::PBEZ_PARAM +  6));
    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec(108.017,  43.336)), module, Bezosc::PBEZ_PARAM +  7));
    addParam(createParamCentered<LargeCyanHoleKnob>(mm2px(Vec(108.017,  57.239)), module, Bezosc::PBEZ_PARAM +  8));
    addParam(createParamCentered<LargeCyanHoleKnob>(mm2px(Vec(108.017,  71.420)), module, Bezosc::PBEZ_PARAM +  9));
    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec(108.017,  85.163)), module, Bezosc::PBEZ_PARAM + 10));
    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec(108.017,  99.212)), module, Bezosc::PBEZ_PARAM + 11));

    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 95.847, 112.443)), module, Bezosc::PBEZ_PARAM + 13)); //note swapped order!!
    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 81.810, 112.443)), module, Bezosc::PBEZ_PARAM + 12));
    addParam(createParamCentered<LargeCyanHoleKnob>(mm2px(Vec( 67.907, 112.443)), module, Bezosc::PBEZ_PARAM + 15));
    addParam(createParamCentered<LargeCyanHoleKnob>(mm2px(Vec( 53.726, 112.443)), module, Bezosc::PBEZ_PARAM + 14));
    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 39.983, 112.443)), module, Bezosc::PBEZ_PARAM + 17));
    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 25.934, 112.443)), module, Bezosc::PBEZ_PARAM + 16));

		addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 13.764,  99.212)), module, Bezosc::PBEZ_PARAM + 19)); //note swapped order!!
    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 13.764,  85.163)), module, Bezosc::PBEZ_PARAM + 18));
    addParam(createParamCentered<LargeCyanHoleKnob>(mm2px(Vec( 13.764,  71.420)), module, Bezosc::PBEZ_PARAM + 21));
    addParam(createParamCentered<LargeCyanHoleKnob>(mm2px(Vec( 13.764,  57.239)), module, Bezosc::PBEZ_PARAM + 20));
    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 13.764,  43.336)), module, Bezosc::PBEZ_PARAM + 23));
    addParam(createParamCentered<LargePaleHoleKnob>(mm2px(Vec( 13.764,  29.299)), module, Bezosc::PBEZ_PARAM + 22));

    addParam(createParamCentered<CyanHoleKnob>(mm2px(Vec(126.998, 16.068)), module, Bezosc::PBEZSCALEX_PARAM));
    addParam(createParamCentered<CyanHoleKnob>(mm2px(Vec(126.998, 25.697)), module, Bezosc::PBEZSCALEY_PARAM));
    addParam(createParamCentered<CyanHoleKnob>(mm2px(Vec(126.998, 35.335)), module, Bezosc::PBEZSCALETH_PARAM));
    addParam(createParamCentered<CyanHoleKnob>(mm2px(Vec(126.998, 44.972)), module, Bezosc::PBEZSCALEL_PARAM));
    addParam(createParamCentered<PaleHoleKnob>(mm2px(Vec(126.998, 61.035)), module, Bezosc::PTANSCALEX_PARAM));
    addParam(createParamCentered<PaleHoleKnob>(mm2px(Vec(126.998, 70.672)), module, Bezosc::PTANSCALEY_PARAM));
    addParam(createParamCentered<PaleHoleKnob>(mm2px(Vec(126.998, 80.31)), module, Bezosc::PTANSCALETH_PARAM));
    addParam(createParamCentered<PaleHoleKnob>(mm2px(Vec(126.998, 89.947)), module, Bezosc::PTANSCALEL_PARAM));

    addParam(createParamCentered<LargeCyanSnapKnob>(mm2px(Vec(111.168, 116.248)), module, Bezosc::MODUS_PARAM));
    addParam(createParamCentered<HugeCyanHoleKnob>(mm2px(Vec(131.853, 110.896)), module, Bezosc::PBEZFREQ_PARAM));

		addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 25.934,  16.068)), module, Bezosc::IBEZ_INPUT +  0));
		addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 39.983,  16.068)), module, Bezosc::IBEZ_INPUT +  1));
		addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 53.726,  16.068)), module, Bezosc::IBEZ_INPUT +  2));
		addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 67.907,  16.068)), module, Bezosc::IBEZ_INPUT +  3));
		addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 81.810,  16.068)), module, Bezosc::IBEZ_INPUT +  4));
		addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 95.847,  16.068)), module, Bezosc::IBEZ_INPUT +  5));

    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec(108.017,  29.299)), module, Bezosc::IBEZ_INPUT +  6));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec(108.017,  43.336)), module, Bezosc::IBEZ_INPUT +  7));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec(108.017,  57.239)), module, Bezosc::IBEZ_INPUT +  8));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec(108.017,  71.420)), module, Bezosc::IBEZ_INPUT +  9));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec(108.017,  85.163)), module, Bezosc::IBEZ_INPUT + 10));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec(108.017,  99.212)), module, Bezosc::IBEZ_INPUT + 11));

    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 95.847, 112.443)), module, Bezosc::IBEZ_INPUT + 13));//note swapped order!!
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 81.810, 112.443)), module, Bezosc::IBEZ_INPUT + 12));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 67.907, 112.443)), module, Bezosc::IBEZ_INPUT + 15));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 53.726, 112.443)), module, Bezosc::IBEZ_INPUT + 14));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 39.983, 112.443)), module, Bezosc::IBEZ_INPUT + 17));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 25.934, 112.443)), module, Bezosc::IBEZ_INPUT + 16));

		addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 13.764,  99.212)), module, Bezosc::IBEZ_INPUT + 19));//note swapped order!!
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 13.764,  85.163)), module, Bezosc::IBEZ_INPUT + 18));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 13.764,  71.420)), module, Bezosc::IBEZ_INPUT + 21));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 13.764,  57.239)), module, Bezosc::IBEZ_INPUT + 20));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 13.764,  43.336)), module, Bezosc::IBEZ_INPUT + 23));
    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec( 13.764,  29.299)), module, Bezosc::IBEZ_INPUT + 22));

    addInput(createInputCentered<PJ301MSPort>(mm2px(Vec(131.853, 110.896)), module, Bezosc::IBEZFREQ_INPUT));

		addOutput(createOutputCentered<PJ301MDPort>(mm2px(Vec(137.158, 16.068)), module, Bezosc::OBEZX_OUTPUT));
		addOutput(createOutputCentered<PJ301MDPort>(mm2px(Vec(137.158, 25.697)), module, Bezosc::OBEZY_OUTPUT));
		addOutput(createOutputCentered<PJ301MDPort>(mm2px(Vec(137.158, 35.335)), module, Bezosc::OBEZTH_OUTPUT));
		addOutput(createOutputCentered<PJ301MDPort>(mm2px(Vec(137.158, 44.972)), module, Bezosc::OBEZL_OUTPUT));
		addOutput(createOutputCentered<PJ301MDPort>(mm2px(Vec(137.158, 61.035)), module, Bezosc::OTANX_OUTPUT));
		addOutput(createOutputCentered<PJ301MDPort>(mm2px(Vec(137.158, 70.672)), module, Bezosc::OTANY_OUTPUT));
		addOutput(createOutputCentered<PJ301MDPort>(mm2px(Vec(137.158, 80.31)), module, Bezosc::OTANTH_OUTPUT));
		addOutput(createOutputCentered<PJ301MDPort>(mm2px(Vec(137.158, 89.947)), module, Bezosc::OTANL_OUTPUT));

    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 25.934,   6.428)), module, Bezosc::LLED_LIGHT +  0));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 39.983,   6.428)), module, Bezosc::LLED_LIGHT +  1));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 53.726,   6.428)), module, Bezosc::LLED_LIGHT +  2));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 67.907,   6.428)), module, Bezosc::LLED_LIGHT +  3));
    addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec( 75.002,   6.428)), module, Bezosc::LLED_LIGHT +  4));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 81.810,   6.428)), module, Bezosc::LLED_LIGHT +  5));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 95.847,   6.428)), module, Bezosc::LLED_LIGHT +  6));

    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(116.838,  29.299)), module, Bezosc::LLED_LIGHT +  7));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(116.838,  43.336)), module, Bezosc::LLED_LIGHT +  8));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(116.838,  57.239)), module, Bezosc::LLED_LIGHT +  9));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(116.838,  71.420)), module, Bezosc::LLED_LIGHT + 10));
    addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(116.838,  77.991)), module, Bezosc::LLED_LIGHT + 11));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(116.838,  85.163)), module, Bezosc::LLED_LIGHT + 12));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(116.838,  99.212)), module, Bezosc::LLED_LIGHT + 13));

    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 95.847, 122.075)), module, Bezosc::LLED_LIGHT + 14));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 81.810, 122.075)), module, Bezosc::LLED_LIGHT + 15));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 67.907, 122.075)), module, Bezosc::LLED_LIGHT + 16));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 53.726, 122.075)), module, Bezosc::LLED_LIGHT + 17));
    addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec( 47.067, 122.075)), module, Bezosc::LLED_LIGHT + 18));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 39.983, 122.075)), module, Bezosc::LLED_LIGHT + 19));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec( 25.934, 122.075)), module, Bezosc::LLED_LIGHT + 20));

    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(  5.078,  99.212)), module, Bezosc::LLED_LIGHT + 21));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(  5.078,  85.163)), module, Bezosc::LLED_LIGHT + 22));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(  5.078,  71.420)), module, Bezosc::LLED_LIGHT + 23));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(  5.078,  57.239)), module, Bezosc::LLED_LIGHT + 24));
    addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(  5.078,  50.209)), module, Bezosc::LLED_LIGHT + 25));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(  5.078,  43.336)), module, Bezosc::LLED_LIGHT + 26));
    addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(  5.078,  29.299)), module, Bezosc::LLED_LIGHT + 27));
	}
};

Model* modelBezosc = createModel<Bezosc, BezoscWidget>("Bezosc");
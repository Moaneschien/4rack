#pragma once
#include "componentlibrary.hpp"
//#include "random.hpp"
//#include <vector>
//#include <jansson.h>
//#include "widget/Widget.hpp"
//#include <iostream>

using namespace std;

namespace rack {

  struct HoleKnob : RoundKnob {
    HoleKnob() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/RampLibrary/HoleKnob.svg")));
    }
  };

}
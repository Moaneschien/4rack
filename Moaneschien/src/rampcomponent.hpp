#pragma once
#include "componentlibrary.hpp"

using namespace std;

namespace rack {

  struct GreyHoleKnob : RoundKnob {
    GreyHoleKnob() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/RampLibrary/GreyHoleKnob.svg")));
    }
  };

}
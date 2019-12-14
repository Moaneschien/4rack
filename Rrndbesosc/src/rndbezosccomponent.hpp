#pragma once
#include "componentlibrary.hpp"

using namespace std;

namespace rack {

  struct GreenKnob : RoundKnob {
    GreenKnob() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/rndbezosclib/greenknob.svg")));
    }
  };
  struct HugeGreenKnob : RoundKnob {
    HugeGreenKnob() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/rndbezosclib/hugegreenknob.svg")));
    }
  };

}
#pragma once
#include "componentlibrary.hpp"
#include <app/SvgSwitch.hpp>

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
      shadow->opacity = 0.0f; // hide shadow
    }
  };

  struct ModusThree : app::SvgSwitch {
    ModusThree() {
      addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/rndbezosclib/select0.svg")));
      addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/rndbezosclib/select1.svg")));
      addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/rndbezosclib/select2.svg")));
    }
  };
}
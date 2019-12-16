#pragma once
#include "componentlibrary.hpp"

using namespace std;

namespace rack {

  struct CyanHoleKnob : RoundKnob {
    CyanHoleKnob() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/BezoscLib/CyanHoleKnob.svg")));
    }
  };
  struct PaleHoleKnob : RoundKnob {
    PaleHoleKnob() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/BezoscLib/PaleHoleKnob.svg")));
    }
  };

  struct LargeCyanHoleKnob : RoundKnob {
    LargeCyanHoleKnob() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/BezoscLib/LargeCyanHoleKnob.svg")));
    }
  };
  struct LargePaleHoleKnob : RoundKnob {
    LargePaleHoleKnob() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/BezoscLib/LargePaleHoleKnob.svg")));
    }
  };
  struct HugeCyanHoleKnob : RoundKnob {
    HugeCyanHoleKnob() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/BezoscLib/HugeCyanHoleKnob.svg")));
    }
  };

  struct PJ301MSPort : SvgPort {
    PJ301MSPort() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/BezoscLib/PJ301MS.svg")));
    }
  };
  struct PJ301MDPort : SvgPort {
    PJ301MDPort() {
      setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/BezoscLib/PJ301MD.svg")));
    }
  };

}
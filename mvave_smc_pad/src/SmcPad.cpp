#include "plugin.hpp"

// Number of controls
static constexpr int NUM_KNOBS = 8;
static constexpr int NUM_PADS = 16;
static constexpr int NUM_BTNS = 4;

// CV range options
enum CvRange {
	RANGE_0_1  = 0,
	RANGE_0_2  = 1,
	RANGE_0_5  = 2,
	RANGE_0_10 = 3,
	NUM_RANGES
};

static float cvRangeMax(CvRange r) {
	switch (r) {
		case RANGE_0_1:  return 1.f;
		case RANGE_0_2:  return 2.f;
		case RANGE_0_5:  return 5.f;
		case RANGE_0_10: return 10.f;
		default:         return 10.f;
	}
}

static const char* cvRangeLabel(CvRange r) {
	switch (r) {
		case RANGE_0_1:  return "0V to 1V";
		case RANGE_0_2:  return "0V to 2V";
		case RANGE_0_5:  return "0V to 5V";
		case RANGE_0_10: return "0V to 10V";
		default:         return "0V to 10V";
	}
}

enum PadMode {
	PAD_GATE = 0,
	PAD_TRIGGER = 1
};

struct SmcPad : Module {
	enum ParamIds {
		KNOB_1_PARAM, KNOB_2_PARAM, KNOB_3_PARAM, KNOB_4_PARAM,
		KNOB_5_PARAM, KNOB_6_PARAM, KNOB_7_PARAM, KNOB_8_PARAM,
		PAD_1_PARAM,  PAD_2_PARAM,  PAD_3_PARAM,  PAD_4_PARAM,
		PAD_5_PARAM,  PAD_6_PARAM,  PAD_7_PARAM,  PAD_8_PARAM,
		PAD_9_PARAM,  PAD_10_PARAM, PAD_11_PARAM, PAD_12_PARAM,
		PAD_13_PARAM, PAD_14_PARAM, PAD_15_PARAM, PAD_16_PARAM,
		BTN_1_PARAM,  BTN_2_PARAM,  BTN_3_PARAM,  BTN_4_PARAM,
		NUM_PARAMS
	};

	enum InputIds { NUM_INPUTS };

	enum OutputIds {
		KNOB_1_OUTPUT, KNOB_2_OUTPUT, KNOB_3_OUTPUT, KNOB_4_OUTPUT,
		KNOB_5_OUTPUT, KNOB_6_OUTPUT, KNOB_7_OUTPUT, KNOB_8_OUTPUT,
		PAD_1_OUTPUT,  PAD_2_OUTPUT,  PAD_3_OUTPUT,  PAD_4_OUTPUT,
		PAD_5_OUTPUT,  PAD_6_OUTPUT,  PAD_7_OUTPUT,  PAD_8_OUTPUT,
		PAD_9_OUTPUT,  PAD_10_OUTPUT, PAD_11_OUTPUT, PAD_12_OUTPUT,
		PAD_13_OUTPUT, PAD_14_OUTPUT, PAD_15_OUTPUT, PAD_16_OUTPUT,
		BTN_1_OUTPUT,  BTN_2_OUTPUT,  BTN_3_OUTPUT,  BTN_4_OUTPUT,
		NUM_OUTPUTS
	};

	enum LightIds {
		ENUMS(PAD_LIGHT, NUM_PADS),
		NUM_LIGHTS
	};

	PadMode padMode[NUM_PADS] = {};
	CvRange padRange[NUM_PADS] = {};
	CvRange knobRange[NUM_KNOBS] = {};
	PadMode btnMode[NUM_BTNS] = {};
	CvRange btnRange[NUM_BTNS] = {};

	dsp::PulseGenerator triggerPulse[NUM_PADS];
	dsp::PulseGenerator btnPulse[NUM_BTNS];
	bool padPrevState[NUM_PADS] = {};
	bool btnPrevState[NUM_BTNS] = {};
	float triggerDuration = 0.001f;

	SmcPad() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		const char* knobLabels[NUM_KNOBS] = {
			"Knob 1", "Knob 2", "Knob 3", "Knob 4",
			"Knob 5", "Knob 6", "Knob 7", "Knob 8"
		};
		for (int i = 0; i < NUM_KNOBS; i++) {
			configParam(KNOB_1_PARAM + i, 0.f, 1.f, 0.f, knobLabels[i]);
			configOutput(KNOB_1_OUTPUT + i, knobLabels[i]);
			knobRange[i] = RANGE_0_10;
		}

		const char* padLabels[NUM_PADS] = {
			"Pad 1",  "Pad 2",  "Pad 3",  "Pad 4",
			"Pad 5",  "Pad 6",  "Pad 7",  "Pad 8",
			"Pad 9",  "Pad 10", "Pad 11", "Pad 12",
			"Pad 13", "Pad 14", "Pad 15", "Pad 16"
		};
		for (int i = 0; i < NUM_PADS; i++) {
			configParam(PAD_1_PARAM + i, 0.f, 1.f, 0.f, padLabels[i]);
			configOutput(PAD_1_OUTPUT + i, padLabels[i]);
			padMode[i] = PAD_GATE;
			padRange[i] = RANGE_0_10;
		}

		const char* btnLabels[NUM_BTNS] = {"Play", "Stop", "Shift", "Note Repeat"};
		for (int i = 0; i < NUM_BTNS; i++) {
			configParam(BTN_1_PARAM + i, 0.f, 1.f, 0.f, btnLabels[i]);
			configOutput(BTN_1_OUTPUT + i, btnLabels[i]);
			btnMode[i] = PAD_GATE;
			btnRange[i] = RANGE_0_10;
		}
	}

	void process(const ProcessArgs& args) override {
		for (int i = 0; i < NUM_KNOBS; i++) {
			float knobVal = params[KNOB_1_PARAM + i].getValue();
			float maxV = cvRangeMax(knobRange[i]);
			outputs[KNOB_1_OUTPUT + i].setVoltage(knobVal * maxV);
		}

		for (int i = 0; i < NUM_PADS; i++) {
			bool pressed = params[PAD_1_PARAM + i].getValue() > 0.5f;
			float maxV = cvRangeMax(padRange[i]);
			if (padMode[i] == PAD_GATE) {
				outputs[PAD_1_OUTPUT + i].setVoltage(pressed ? maxV : 0.f);
			} else {
				if (pressed && !padPrevState[i])
					triggerPulse[i].trigger(triggerDuration);
				bool pulseActive = triggerPulse[i].process(args.sampleTime);
				outputs[PAD_1_OUTPUT + i].setVoltage(pulseActive ? maxV : 0.f);
			}
			padPrevState[i] = pressed;
			lights[PAD_LIGHT + i].setBrightness(outputs[PAD_1_OUTPUT + i].getVoltage() / maxV);
		}

		for (int i = 0; i < NUM_BTNS; i++) {
			bool pressed = params[BTN_1_PARAM + i].getValue() > 0.5f;
			float maxV = cvRangeMax(btnRange[i]);
			if (btnMode[i] == PAD_GATE) {
				outputs[BTN_1_OUTPUT + i].setVoltage(pressed ? maxV : 0.f);
			} else {
				if (pressed && !btnPrevState[i])
					btnPulse[i].trigger(triggerDuration);
				bool pulseActive = btnPulse[i].process(args.sampleTime);
				outputs[BTN_1_OUTPUT + i].setVoltage(pulseActive ? maxV : 0.f);
			}
			btnPrevState[i] = pressed;
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_t* padModesJ = json_array();
		for (int i = 0; i < NUM_PADS; i++)
			json_array_append_new(padModesJ, json_integer(padMode[i]));
		json_object_set_new(rootJ, "padModes", padModesJ);

		json_t* padRangesJ = json_array();
		for (int i = 0; i < NUM_PADS; i++)
			json_array_append_new(padRangesJ, json_integer(padRange[i]));
		json_object_set_new(rootJ, "padRanges", padRangesJ);

		json_t* knobRangesJ = json_array();
		for (int i = 0; i < NUM_KNOBS; i++)
			json_array_append_new(knobRangesJ, json_integer(knobRange[i]));
		json_object_set_new(rootJ, "knobRanges", knobRangesJ);

		json_t* btnModesJ = json_array();
		for (int i = 0; i < NUM_BTNS; i++)
			json_array_append_new(btnModesJ, json_integer(btnMode[i]));
		json_object_set_new(rootJ, "btnModes", btnModesJ);

		json_t* btnRangesJ = json_array();
		for (int i = 0; i < NUM_BTNS; i++)
			json_array_append_new(btnRangesJ, json_integer(btnRange[i]));
		json_object_set_new(rootJ, "btnRanges", btnRangesJ);

		json_object_set_new(rootJ, "triggerDuration", json_real(triggerDuration));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* j;

		j = json_object_get(rootJ, "padModes");
		if (j) for (int i = 0; i < NUM_PADS; i++) {
			json_t* v = json_array_get(j, i);
			if (v) padMode[i] = (PadMode)json_integer_value(v);
		}

		j = json_object_get(rootJ, "padRanges");
		if (j) for (int i = 0; i < NUM_PADS; i++) {
			json_t* v = json_array_get(j, i);
			if (v) padRange[i] = (CvRange)clamp((int)json_integer_value(v), 0, NUM_RANGES - 1);
		}

		j = json_object_get(rootJ, "knobRanges");
		if (j) for (int i = 0; i < NUM_KNOBS; i++) {
			json_t* v = json_array_get(j, i);
			if (v) knobRange[i] = (CvRange)clamp((int)json_integer_value(v), 0, NUM_RANGES - 1);
		}

		j = json_object_get(rootJ, "btnModes");
		if (j) for (int i = 0; i < NUM_BTNS; i++) {
			json_t* v = json_array_get(j, i);
			if (v) btnMode[i] = (PadMode)json_integer_value(v);
		}

		j = json_object_get(rootJ, "btnRanges");
		if (j) for (int i = 0; i < NUM_BTNS; i++) {
			json_t* v = json_array_get(j, i);
			if (v) btnRange[i] = (CvRange)clamp((int)json_integer_value(v), 0, NUM_RANGES - 1);
		}

		j = json_object_get(rootJ, "triggerDuration");
		if (j) triggerDuration = (float)json_real_value(j);
	}
};

// -- Custom widgets -------------------------------------------------------

struct SmcKnob : RoundKnob {
	SmcKnob() {
		setSvg(Svg::load(asset::system("res/ComponentLibrary/RoundBlackKnob.svg")));
	}
};

struct SmcPadButton : app::SvgSwitch {
	SmcPadButton() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/pad_off.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/pad_on.svg")));
	}
};

// -- Layout constants (19HP = 96.52mm) ------------------------------------
// 7 columns: [knob1] | [knob2] | [pad1] [pad2] [pad3] [pad4] | [btn]

static constexpr float COL_KNOB[2] = {8.f, 19.f};
static constexpr float COL_PAD[4]  = {32.f, 45.f, 58.f, 71.f};
static constexpr float COL_BTN     = 84.f;

// 4 rows, spread vertically
static constexpr float ROW_Y[4] = {20.f, 46.f, 72.f, 98.f};

// Output jack offset (directly below each control)
static constexpr float JACK_DY = 11.f;

// Vertical divider X positions
static constexpr float DIV_X[] = {13.5f, 25.5f, 38.5f, 51.5f, 64.5f, 77.5f};
static constexpr int NUM_DIVS = 6;

// Index mapping (physical: bottom-to-top numbering)
static constexpr int KNOB_IDX_MAP[4][2] = {{6,7}, {4,5}, {2,3}, {0,1}};
static constexpr int PAD_ROW_START[4] = {12, 8, 4, 0};

// -- Panel labels ---------------------------------------------------------

struct SmcPadLabels : TransparentWidget {
	void draw(const DrawArgs& args) override {
		NVGcontext* vg = args.vg;

		// Column dividers only (no text labels)
		nvgStrokeColor(vg, nvgRGBA(80, 80, 80, 80));
		nvgStrokeWidth(vg, 0.5f);
		for (int i = 0; i < NUM_DIVS; i++) {
			nvgBeginPath(vg);
			nvgMoveTo(vg, mm2px(DIV_X[i]), mm2px(11.f));
			nvgLineTo(vg, mm2px(DIV_X[i]), mm2px(118.f));
			nvgStroke(vg);
		}
	}
};

// -- Module Widget --------------------------------------------------------

struct SmcPadWidget : ModuleWidget {
	explicit SmcPadWidget(SmcPad* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/SmcPad.svg")));

		// Corner screws
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Labels overlay
		auto* labels = createWidget<SmcPadLabels>(Vec(0, 0));
		labels->box.size = box.size;
		addChild(labels);

		// -- KNOBS: 2 columns x 4 rows --
		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 2; col++) {
				int idx = KNOB_IDX_MAP[row][col];
				addParam(createParamCentered<RoundBlackKnob>(
					mm2px(Vec(COL_KNOB[col], ROW_Y[row])),
					module, SmcPad::KNOB_1_PARAM + idx));
				addOutput(createOutputCentered<PJ301MPort>(
					mm2px(Vec(COL_KNOB[col], ROW_Y[row] + JACK_DY)),
					module, SmcPad::KNOB_1_OUTPUT + idx));
			}
		}

		// -- PADS: 4 columns x 4 rows --
		for (int row = 0; row < 4; row++) {
			for (int col = 0; col < 4; col++) {
				int idx = PAD_ROW_START[row] + col;
				addParam(createParamCentered<SmcPadButton>(
					mm2px(Vec(COL_PAD[col], ROW_Y[row])),
					module, SmcPad::PAD_1_PARAM + idx));
				addOutput(createOutputCentered<PJ301MPort>(
					mm2px(Vec(COL_PAD[col], ROW_Y[row] + JACK_DY)),
					module, SmcPad::PAD_1_OUTPUT + idx));
			}
		}

		// -- RIGHT BUTTONS: 1 column x 4 rows --
		for (int i = 0; i < NUM_BTNS; i++) {
			addParam(createParamCentered<VCVButton>(
				mm2px(Vec(COL_BTN, ROW_Y[i])),
				module, SmcPad::BTN_1_PARAM + i));
			addOutput(createOutputCentered<PJ301MPort>(
				mm2px(Vec(COL_BTN, ROW_Y[i] + JACK_DY)),
				module, SmcPad::BTN_1_OUTPUT + i));
		}
	}

	void appendContextMenu(Menu* menu) override {
		SmcPad* module = getModule<SmcPad>();
		if (!module) return;

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Knob CV Ranges"));

		const char* knobNames[NUM_KNOBS] = {
			"Knob 1", "Knob 2", "Knob 3", "Knob 4",
			"Knob 5", "Knob 6", "Knob 7", "Knob 8"
		};
		for (int i = 0; i < NUM_KNOBS; i++) {
			menu->addChild(createSubmenuItem(knobNames[i], cvRangeLabel(module->knobRange[i]),
				[=](Menu* cm) {
					for (int r = 0; r < NUM_RANGES; r++)
						cm->addChild(createCheckMenuItem(cvRangeLabel((CvRange)r), "",
							[=]() { return module->knobRange[i] == (CvRange)r; },
							[=]() { module->knobRange[i] = (CvRange)r; }));
				}));
		}

		menu->addChild(new MenuSeparator);
		menu->addChild(createSubmenuItem("Set All Knob Ranges", "",
			[=](Menu* cm) {
				for (int r = 0; r < NUM_RANGES; r++)
					cm->addChild(createMenuItem(cvRangeLabel((CvRange)r), "",
						[=]() { for (int i = 0; i < NUM_KNOBS; i++) module->knobRange[i] = (CvRange)r; }));
			}));

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Pad Settings"));

		const char* padNames[NUM_PADS] = {
			"Pad 1",  "Pad 2",  "Pad 3",  "Pad 4",
			"Pad 5",  "Pad 6",  "Pad 7",  "Pad 8",
			"Pad 9",  "Pad 10", "Pad 11", "Pad 12",
			"Pad 13", "Pad 14", "Pad 15", "Pad 16"
		};
		for (int i = 0; i < NUM_PADS; i++) {
			std::string info = std::string(module->padMode[i] == PAD_GATE ? "Gate" : "Trigger")
				+ " | " + cvRangeLabel(module->padRange[i]);
			menu->addChild(createSubmenuItem(padNames[i], info,
				[=](Menu* cm) {
					cm->addChild(createMenuLabel("Mode"));
					cm->addChild(createCheckMenuItem("Gate", "",
						[=]() { return module->padMode[i] == PAD_GATE; },
						[=]() { module->padMode[i] = PAD_GATE; }));
					cm->addChild(createCheckMenuItem("Trigger", "",
						[=]() { return module->padMode[i] == PAD_TRIGGER; },
						[=]() { module->padMode[i] = PAD_TRIGGER; }));
					cm->addChild(new MenuSeparator);
					cm->addChild(createMenuLabel("CV Range"));
					for (int r = 0; r < NUM_RANGES; r++)
						cm->addChild(createCheckMenuItem(cvRangeLabel((CvRange)r), "",
							[=]() { return module->padRange[i] == (CvRange)r; },
							[=]() { module->padRange[i] = (CvRange)r; }));
				}));
		}

		menu->addChild(new MenuSeparator);
		menu->addChild(createSubmenuItem("Set All Pads Mode", "",
			[=](Menu* cm) {
				cm->addChild(createMenuItem("All Gate", "",
					[=]() { for (int i = 0; i < NUM_PADS; i++) module->padMode[i] = PAD_GATE; }));
				cm->addChild(createMenuItem("All Trigger", "",
					[=]() { for (int i = 0; i < NUM_PADS; i++) module->padMode[i] = PAD_TRIGGER; }));
			}));
		menu->addChild(createSubmenuItem("Set All Pad Ranges", "",
			[=](Menu* cm) {
				for (int r = 0; r < NUM_RANGES; r++)
					cm->addChild(createMenuItem(cvRangeLabel((CvRange)r), "",
						[=]() { for (int i = 0; i < NUM_PADS; i++) module->padRange[i] = (CvRange)r; }));
			}));

		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Button Settings"));

		const char* btnNames[NUM_BTNS] = {"Play", "Stop", "Shift", "Note Repeat"};
		for (int i = 0; i < NUM_BTNS; i++) {
			std::string info = std::string(module->btnMode[i] == PAD_GATE ? "Gate" : "Trigger")
				+ " | " + cvRangeLabel(module->btnRange[i]);
			menu->addChild(createSubmenuItem(btnNames[i], info,
				[=](Menu* cm) {
					cm->addChild(createMenuLabel("Mode"));
					cm->addChild(createCheckMenuItem("Gate", "",
						[=]() { return module->btnMode[i] == PAD_GATE; },
						[=]() { module->btnMode[i] = PAD_GATE; }));
					cm->addChild(createCheckMenuItem("Trigger", "",
						[=]() { return module->btnMode[i] == PAD_TRIGGER; },
						[=]() { module->btnMode[i] = PAD_TRIGGER; }));
					cm->addChild(new MenuSeparator);
					cm->addChild(createMenuLabel("CV Range"));
					for (int r = 0; r < NUM_RANGES; r++)
						cm->addChild(createCheckMenuItem(cvRangeLabel((CvRange)r), "",
							[=]() { return module->btnRange[i] == (CvRange)r; },
							[=]() { module->btnRange[i] = (CvRange)r; }));
				}));
		}
	}
};

Model* modelSmcPad = createModel<SmcPad, SmcPadWidget>("SmcPad");

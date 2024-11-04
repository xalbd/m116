// my_predictor.h
// This file contains a sample my_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.
// Note that this predictor doesn't use the whole 32 kilobytes available
// for the CBP-2 contest; it is just an example.
#ifndef MY_PREDICTOR_H
#define MY_PREDICTOR_H

static const uint32_t TAG_LENGTH = 9;
static const uint32_t INDEX_LENGTH = 32 - TAG_LENGTH;
static const uint32_t COMPONENT_SIZE = 1 << INDEX_LENGTH;

class my_update : public branch_update {
public:
	unsigned int pc;
};

class my_predictor : public branch_predictor {
private:
	class Tage {
	public:
		Tage() : history(0) {
			memset(bimodal, 0, sizeof bimodal);
		}

		bool predict(uint32_t pc) {
			pred_component = tagMatchesPredictor(pc);
			altpred_component = 0;

			pred = predictComponent(pc, pred_component);
			altpred = predictComponent(pc, altpred_component);

			return pred;
		}

		void update(uint32_t pc, bool taken) {
			if (pred_component > 0) {
				updatePredictor(pc, taken);
			} else {
				updateBimodal(pc, taken);

				if (pred != taken) {
					uint32_t predictorIndex = getPredictorIndex(pc);
					if (predictor[predictorIndex].useful == 0) {
						predictor[predictorIndex].pred = 0b100;
						predictor[predictorIndex].tag = getPredictorTag(pc);
					}
				}
			}

			// update history
			history <<= 1;
			history |= taken;
		}

	private:
		struct PredictorEntry {
			PredictorEntry() {
				pred = 0b100;
				tag = 0;
				useful = 0;
			}

			uint8_t pred; // 3 bit prediction
			uint32_t tag;
			uint8_t useful; // 2 bit useful
		};

		uint8_t bimodal[COMPONENT_SIZE]; // holds 2 bit bimodal counter
		PredictorEntry predictor[COMPONENT_SIZE];
		uint32_t history;

		uint32_t pred_component;
		uint32_t altpred_component;
		bool pred;
		bool altpred;

		static uint32_t getBimodalIndex(const uint32_t pc) {
			return pc & ((1 << INDEX_LENGTH) - 1);
		}

		bool predictBimodal(const uint32_t pc) const {
			uint32_t index = getBimodalIndex(pc);
			return bimodal[index] >> 1;
		}

		void updateBimodal(const uint32_t pc, bool taken) {
			// update bimodal predictor
			uint32_t index = getBimodalIndex(pc);
			if (taken && bimodal[index] < 0b11) {
				bimodal[index]++;
			} else if (!taken && bimodal[index] > 0b00) {
				bimodal[index]--;
			}
		}

		bool predictComponent(uint32_t pc, uint32_t component) {
			return predictor[getPredictorIndex(pc)].pred >> 2;
		}

		uint32_t getPredictorIndex(uint32_t pc) {
			return getBimodalIndex(pc) ^ (history & ((1 << INDEX_LENGTH) - 1));
		}

		uint32_t getPredictorTag(uint32_t pc) {
			return pc >> INDEX_LENGTH;
		}

		bool tagMatchesPredictor(uint32_t pc) {
			return predictor[getPredictorIndex(pc)].tag == getPredictorTag(pc);
		}

		void updatePredictor(uint32_t pc, bool taken) {
			uint32_t index = getPredictorIndex(pc);
			if (taken && predictor[index].pred < 0b111) {
				predictor[index].pred++;
			} else if (!taken && predictor[index].pred > 0b000) {
				predictor[index].pred--;
			}

			if (pred != altpred) {
				if (pred == taken && predictor[index].useful < 0b11) {
					predictor[index].useful++;
				} else if (pred != taken && predictor[index].useful > 0b00) {
					predictor[index].useful--;
				}
			}
		}
	};
	Tage tage;

public:
	my_update u;
	branch_info bi;

	my_predictor(void) : u(), bi() {}

	branch_update *predict(branch_info &b) {
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) {
			u.direction_prediction(tage.predict(b.address));
		} else {
			u.direction_prediction(true);
		}
		u.target_prediction(0);
		u.pc = b.address;
		return &u;
	}

	void update(branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			tage.update(static_cast<my_update *>(u)->pc, taken);
		}
	}
};

#endif // MY_PREDICTOR_H

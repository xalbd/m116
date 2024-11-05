// my_predictor.h
// This file contains a sample my_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.
// Note that this predictor doesn't use the whole 32 kilobytes available
// for the CBP-2 contest; it is just an example.
#ifndef MY_PREDICTOR_H
#define MY_PREDICTOR_H

#include <algorithm>
#include <random>
using namespace std;


static const uint32_t TAG_LENGTH = 9;
static const uint32_t INDEX_LENGTH = 10;
static const uint32_t COMPONENT_SIZE = 1 << INDEX_LENGTH;
static const uint32_t COMPONENT_COUNT = 5;
static const uint32_t HISTORY_LENGTH[COMPONENT_COUNT] = {5, 9, 15, 25, 44};
static const uint32_t HISTORY_BUFFER_LENGTH = HISTORY_LENGTH[COMPONENT_COUNT - 1];

class my_update : public branch_update {
public:
	unsigned int pc;
};

class my_predictor : public branch_predictor {
private:
	class Tage {
	public:
		Tage() : history(0), pred_component(0), altpred_component(0), pred(false), altpred(false) {
			memset(bimodal, 0, sizeof bimodal);
		}

		bool predict(uint32_t pc) {
			pred_component = bestMatchComponentBounded(pc, COMPONENT_COUNT + 1);
			altpred_component = bestMatchComponentBounded(pc, pred_component);

			pred = predictComponent(pc, pred_component);
			altpred = predictComponent(pc, altpred_component);

			return pred;
		}

		void update(uint32_t pc, bool taken) {
			if (pred_component > 0) {
				updatePredictor(pc, taken);
			} else {
				updateBimodal(pc, taken);
			}

			if (pred != taken) {
				int start = pred_component + 1;
				int rand = random();

				// not sure how this works
				if (rand & 1) {
					start++;
					if (rand & 2) {
						start++;
						if (rand & 4) {
							start++;
						}
					}
				}



				for (int i = start; i <= COMPONENT_COUNT; i++) {
					uint32_t predictorIndex = getPredictorIndex(pc, i);
					PredictorEntry* e = &predictors[i - 1][predictorIndex];
					if (e->useful == 0) {
						e->pred = 0b100;
						e->tag = getPredictorTag(pc, i);
						break;
					}
				}

			}

			// update history
			history <<= 1;
			history|= taken;
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
		PredictorEntry predictors[COMPONENT_COUNT][COMPONENT_SIZE];
		uint64_t history;

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
			if (component > 0) {
				return predictors[component - 1][getPredictorIndex(pc, component)].pred >> 2;
			} else {
				return predictBimodal(pc);
			}
		}

		uint32_t compressHistory(const uint32_t from, const uint32_t to) {
			uint32_t out = 0;
			uint32_t temp = 0;

			for (int i = 0; i < from; i++) {
				if (i % to == 0) {
					out ^= temp;
					temp = 0;
				}
				temp = (temp << 1) | ((history >> i) & 1);
			}
			out ^= temp;
			return out;
		}

		uint32_t getPredictorIndex(uint32_t pc, uint32_t component) {
			uint32_t compressed_history = compressHistory(HISTORY_LENGTH[component - 1], INDEX_LENGTH);
			return (compressed_history ^ pc ^ (pc >> (INDEX_LENGTH - component) + 1)) & ((1 << INDEX_LENGTH) - 1);
		}

		uint32_t getPredictorTag(uint32_t pc, uint32_t component) {
			uint32_t compressed_history = compressHistory(HISTORY_LENGTH[component - 1], TAG_LENGTH);
			return (compressed_history ^ pc) & ((1 << TAG_LENGTH) - 1);
		}

		bool tagMatchesPredictor(uint32_t pc, uint32_t component) {
			return predictors[component - 1][getPredictorIndex(pc, component)].tag == getPredictorTag(pc, component);
		}

		uint32_t bestMatchComponentBounded(uint32_t pc, uint32_t highest_allowable_component) {
			for (int i = highest_allowable_component - 1; i >= 1; i--) {
				if (tagMatchesPredictor(pc, i)) {
					return i;
				}
			}
			return 0;
		}

		void updatePredictor(uint32_t pc, bool taken) {
			uint32_t index = getPredictorIndex(pc, pred_component);
			PredictorEntry* e = &predictors[pred_component - 1][index];
			if (taken && e->pred < 0b111) {
				e->pred++;
			} else if (!taken && e->pred > 0b000) {
				e->pred--;
			}

			if (pred != altpred) {
				if (pred == taken && e->useful < 0b11) {
					e->useful++;
				} else if (pred != taken && e->useful > 0b00) {
					e->useful--;
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

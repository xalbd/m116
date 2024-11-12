#ifndef MY_PREDICTOR_H
#define MY_PREDICTOR_H

#include <algorithm>
#include <random>
using namespace std;


static const uint32_t INDEX_LENGTH = 9;
static const uint32_t COMPONENT_SIZE = 1 << INDEX_LENGTH;
static const uint32_t BIMODAL_INDEX_LENGTH = 12;
static const uint32_t BIMODAL_SIZE = 1 << BIMODAL_INDEX_LENGTH;
static const uint32_t COMPONENT_COUNT = 7;
static const uint32_t TAG_LENGTH[COMPONENT_COUNT] = {9, 9, 10, 10, 11, 11, 12};
static const uint32_t HISTORY_LENGTH[COMPONENT_COUNT] = {5, 9, 15, 25, 44, 76, 130};
static const uint32_t HISTORY_BUFFER_LENGTH = 150;
static const uint32_t USEFUL_RESET_INTERVAL = 256000;

class my_update : public branch_update {
public:
	unsigned int pc;
};

class my_predictor : public branch_predictor {
private:
	class Tage {
	public:
		Tage() : num_branches(0), use_alt_on_na(0), strong(false), pred_component(0), altpred_component(0), pred(false),
		         altpred(false), outpred(false) {
			memset(bimodal, 2, sizeof bimodal);
			memset(history, 0, sizeof history);
		}

		void add_non_conditional_branch() {
			// update history
			for (int i = HISTORY_BUFFER_LENGTH - 1; i >= 1; i--) {
				history[i] = history[i - 1];
			}
			history[0] = true;
		}

		bool predict(uint32_t pc) {
			pred_component = bestMatchingComponent(pc, COMPONENT_COUNT + 1);
			altpred_component = bestMatchingComponent(pc, pred_component);

			pred = predictComponent(pc, pred_component);
			altpred = predictComponent(pc, altpred_component);

			if (pred_component == 0) {
				outpred = pred;
			} else {
				int pred_pred = predictors[pred_component - 1][getComponentIndex(pc, pred_component)].pred;
				strong = !(pred_pred == 0b100 || pred_pred == 0b011);
				if (use_alt_on_na < 8 || strong) {
					outpred = pred;
				} else {
					outpred = altpred;
				}
			}

			return outpred;
		}

		void update(uint32_t pc, bool taken) {
			if (pred_component > 0) {
				updatePredictor(pc, taken);
			} else {
				updateBimodal(pc, taken);
			}

			// attempt to allocate if prediction is incorrect
			if (outpred != taken && pred_component != COMPONENT_COUNT) {
				const int start = pred_component + 1;

				can_allocate.clear();
				for (int i = start; i <= COMPONENT_COUNT; i++) {
					PredictorEntry *e = &predictors[i - 1][getComponentIndex(pc, i)];
					if (e->useful == 0) {
						can_allocate.push_back(i);
					}
				}

				if (can_allocate.size() == 0) {
					for (int i = start; i <= COMPONENT_COUNT; i++) {
						predictors[i - 1][getComponentIndex(pc, i)].useful--;
					}
				} else {
					int i = 0;
					while (random() & 1) {
						i++;
					}

					int component = can_allocate[i % can_allocate.size()];
					PredictorEntry *e = &predictors[component - 1][getComponentIndex(pc, component)];
					e->pred = 0b100;
					e->tag = getComponentTag(pc, component);
				}
			}

			// update history
			for (int i = HISTORY_BUFFER_LENGTH - 1; i >= 1; i--) {
				history[i] = history[i - 1];
			}
			history[0] = taken;

			// reset useful counters
			num_branches++;
			if (num_branches == USEFUL_RESET_INTERVAL) {
				num_branches = 0;
				for (int i = 0; i < COMPONENT_COUNT; i++) {
					for (int j = 0; j < COMPONENT_SIZE; j++) {
						predictors[i][j].useful >>= 1;
					}
				}
			}
		}

	private:
		struct PredictorEntry {
			PredictorEntry() {
				pred = 0b100;
				tag = 0;
				useful = 0;
			}

			uint8_t pred; // 3 bit prediction (>100: yes, <= 011: no)
			uint16_t tag;
			uint8_t useful; // 2 bit useful
		};

		uint8_t bimodal[BIMODAL_SIZE]; // holds 2 bit bimodal
		PredictorEntry predictors[COMPONENT_COUNT][COMPONENT_SIZE];
		bool history[HISTORY_BUFFER_LENGTH];
		int num_branches;
		int use_alt_on_na; // 4 bits, <8 = don't use alt on new alloc; >=8 = use alt on new alloc
		bool strong;
		vector<int> can_allocate;

		int pred_component;
		int altpred_component;
		bool pred;
		bool altpred;
		bool outpred;

		static uint16_t getBimodalIndex(const uint32_t pc) {
			return pc & ((1 << BIMODAL_INDEX_LENGTH) - 1);
		}

		bool predictBimodal(const uint32_t pc) const {
			return bimodal[getBimodalIndex(pc)] >> 1;
		}

		void updateBimodal(const uint32_t pc, bool taken) {
			// update bimodal predictor
			uint16_t index = getBimodalIndex(pc);
			if (taken && bimodal[index] < 0b11) {
				bimodal[index]++;
			} else if (!taken && bimodal[index] > 0b00) {
				bimodal[index]--;
			}
		}

		bool predictComponent(uint32_t pc, int component) const {
			if (component > 0) {
				return predictors[component - 1][getComponentIndex(pc, component)].pred >> 2;
			} else {
				return predictBimodal(pc);
			}
		}

		uint32_t compressHistory(const int from, const int to) const {
			uint32_t out = 0;
			uint32_t temp = 0;

			for (int i = 0; i < from; i++) {
				if (i % to == 0) {
					out ^= temp;
					temp = 0;
				}
				temp = (temp << 1) | history[i];
			}
			out ^= temp;
			return out;
		}

		uint16_t getComponentIndex(uint32_t pc, int component) const {
			uint32_t compressed_history = compressHistory(HISTORY_LENGTH[component - 1], INDEX_LENGTH);
			return (compressed_history ^ pc ^ (pc >> (INDEX_LENGTH - component) + 1)) & ((1 << INDEX_LENGTH) - 1);
		}

		uint16_t getComponentTag(uint32_t pc, int component) const {
			uint32_t compressed_history = compressHistory(HISTORY_LENGTH[component - 1], TAG_LENGTH[component - 1]);
			return (compressed_history ^ pc) & ((1 << TAG_LENGTH[component - 1]) - 1);
		}

		bool tagMatchesComponent(uint32_t pc, int component) const {
			return predictors[component - 1][getComponentIndex(pc, component)].tag == getComponentTag(pc, component);
		}

		int bestMatchingComponent(uint32_t pc, int highest_allowable_component) const {
			for (int i = highest_allowable_component - 1; i >= 1; i--) {
				if (tagMatchesComponent(pc, i)) {
					return i;
				}
			}
			return 0;
		}

		void updatePredictor(uint32_t pc, bool taken) {
			// update use_alt_on_na 4 bit counter
			if (!strong && pred != altpred) {
				if (use_alt_on_na < 16 && pred != taken) {
					use_alt_on_na++;
				} else if (use_alt_on_na > 0 && pred == taken) {
					use_alt_on_na--;
				}
			}


			PredictorEntry *e = &predictors[pred_component - 1][getComponentIndex(pc, pred_component)];

			// update alternate if current not useful
			if (e->useful == 0) {
				if (altpred_component == 0) {
					updateBimodal(pc, taken);
				} else {
					PredictorEntry *alte = &predictors[altpred_component - 1][getComponentIndex(pc, altpred_component)];
					if (taken && alte->pred < 0b111) {
						alte->pred++;
					} else if (!taken && alte->pred > 0b000) {
						alte->pred--;
					}
				}
			}

			// update current counter
			if (taken && e->pred < 0b111) {
				e->pred++;
			} else if (!taken && e->pred > 0b000) {
				e->pred--;
			}

			// update useful if alternate differs in prediction
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

	my_predictor(void) : u(), bi() {
	}

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
		} else {
			tage.add_non_conditional_branch();
		}
	}
};

#endif // MY_PREDICTOR_H

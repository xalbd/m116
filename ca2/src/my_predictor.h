#ifndef MY_PREDICTOR_H
#define MY_PREDICTOR_H

#include <algorithm>
#include <random>
using namespace std;


static const uint32_t INDEX_LENGTH = 10;
static const uint32_t COMPONENT_SIZE = 1 << INDEX_LENGTH;
static const uint32_t BIMODAL_INDEX_LENGTH = 12;
static const uint32_t BIMODAL_SIZE = 1 << BIMODAL_INDEX_LENGTH;
static const uint32_t COMPONENT_COUNT = 4;
static const uint32_t TAG_LENGTH[COMPONENT_COUNT] = {8, 8, 9, 9};
static const uint32_t HISTORY_LENGTH[COMPONENT_COUNT] = {5, 15, 44, 130};
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
		Tage() : num_branches(0), pred_component(0), altpred_component(0), pred(false), altpred(false) {
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

			return pred;
		}

		void update(uint32_t pc, bool taken) {
			if (pred_component > 0) {
				updatePredictor(pc, taken);
			} else {
				updateBimodal(pc, taken);
			}

			// attempt to allocate if prediction is incorrect
			if (pred != taken && pred_component != COMPONENT_COUNT) {
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
						predictors[i][j].useful &= 0b01;
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

			uint8_t pred; // 3 bit prediction
			uint16_t tag;
			uint8_t useful; // 2 bit useful
		};

		uint8_t bimodal[BIMODAL_SIZE]; // holds 2 bit bimodal
		PredictorEntry predictors[COMPONENT_COUNT][COMPONENT_SIZE];
		bool history[HISTORY_BUFFER_LENGTH];
		int num_branches;
		vector<int> can_allocate;

		int pred_component;
		int altpred_component;
		bool pred;
		bool altpred;

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
			PredictorEntry *e = &predictors[pred_component - 1][getComponentIndex(pc, pred_component)];
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

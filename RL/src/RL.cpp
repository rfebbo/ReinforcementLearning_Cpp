#include "RL.h"

using namespace std::chrono;

RL::RL(long n_states, int n_actions, rl_agent_params rlap)
    : num_states(n_states), num_actions(n_actions), p(rlap) {
  this->init();
}

void RL::init() {
  explore = p.explore_start;

  this->reduction = (p.explore_start - p.explore_end) / p.num_episodes;

  for (long i = 0; i < num_states * num_actions; i++) {
    this->Q.push_back(((rand() / static_cast<double>(RAND_MAX)) * 2) - 1);
    // printf("Q %i %f\n", i, Q.back());
  }

  printf("Q size(%i): %.2lf MB\n", Q.size(),
         (static_cast<double>(Q.size()) * sizeof(double)) / 1048576);

  cpu_time = nanoseconds::zero();
  total_cpu_time = milliseconds::zero();
}

void RL::new_episode() {
  eps_elapsed++;

  avg_cpu_time += ((cpu_time.count() / time) - avg_cpu_time) / eps_elapsed;
  avg_action += ((total_action / time) - avg_action) / eps_elapsed;
  avg_time_alive += (time - avg_time_alive) / eps_elapsed;
  avg_rand_actions +=
      ((total_rand_actions / time) - avg_rand_actions) / eps_elapsed;

  if (time > max_time_alive) {
    max_time_alive = time;
    best_run = current_run;
  }

  if (explore > p.explore_end)
    explore -= reduction;

  time = 0;
  num_steps = 0;
  total_cpu_time += duration_cast<milliseconds>(cpu_time);
  cpu_time = nanoseconds::zero();
  total_action = 0;
  total_rand_actions = 0;
  current_run.clear();
}

void RL::reset_averages() {
  avg_cpu_time = 0;
  avg_action = 0;
  avg_rand_actions = 0;
  avg_time_alive = 0;
  eps_elapsed = 0;
}

int RL::get_action(long state) {

  auto start = steady_clock::now();

  int action = -1;
  bool rand_action = false;
  double randn = rand() / static_cast<double>(RAND_MAX);

  if (randn < explore) {
    total_rand_actions++;
    rand_action = true;
  }

  bool found_actual_max_q = false;
  double actual_max_q;
  for (int i = 0; i < num_actions; i++) {
    int idx = state * num_actions + i;
    if (!found_actual_max_q || Q[idx] > actual_max_q) {
      actual_max_q = Q[idx];
      action = i;
      found_actual_max_q = true;
    }
  }

  if (rand_action) {
    action = rand() % 3;
  }

  num_steps++;
  total_action += action;
  time += TIMESTEP;
  current_run.push_back(action);

  auto end = steady_clock::now();

  auto duration = duration_cast<nanoseconds>(end - start);
  cpu_time += duration;

  return action;
}

void RL::update_q(long prev_state, long cur_state, long prev_action,
                  long reward, bool done) {

  bool found_actual_max_q = false;
  double max_q;
  for (int i = 0; i < num_actions; i++) {
    int idx = cur_state * num_actions + i;
    if (!found_actual_max_q || Q[idx] > max_q) {
      max_q = Q[idx];
      found_actual_max_q = true;
    }
  }

  if (!done) {
    double delta =
        p.learning_rate * (reward + p.discount * max_q -
                           Q[prev_state * num_actions + prev_action]);

    Q[prev_state * num_actions + prev_action] += delta;

  } else {
    Q[prev_state * num_actions + prev_action] = reward;
  }
}

void RL::print_params(FILE *f) {
  fprintf(f,
          "NUM_EPISODES %i\n"
          "EXPLORE_START %lf\n"
          "EXPLORE_END %lf\n"
          "DISCOUNT %lf\n"
          "LEARNING_RATE %lf\n\n",
          this->p.num_episodes, this->p.explore_start, this->p.explore_end,
          this->p.discount, this->p.learning_rate);
}
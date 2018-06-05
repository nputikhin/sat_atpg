#include "circuit_graph.h"
#include "iscas89_parser.h"
#include "circuit_to_cnf.h"
#include "fault_cnf.h"
#include "fault_manager.h"
#include "sat/sat_solver.h"
#include "solver_proxy.h"

#include "util/log.h"
#include "util/timer.h"

#include <fstream>
#include <algorithm>

struct Config
{
	uint64_t total_time_limit_s = 0;

	bool write_faults = 0;
	bool write_solutions = 0;
	bool write_detectability = 0;
	bool do_solve = 1;
	bool write_stats = 1;
	bool short_stats = 0;
	float threshold_ratio = 0.6f;
} g_config;

int main(int argc, char* argv[])
{
	if (argc < 2) {
		log_error() << "no input file specified";
		return 1;
	}
	std::ifstream ifs(argv[1]);
	if (!ifs.good()) {
		log_error() << "can't open file" << argv[1];
		return 1;
	}

	CircuitGraph graph;
	Iscas89Parser parser;
	if (!parser.parse(ifs, graph)) {
		log_error() << "can't parse file" << argv[1];
		return 1;
	}

	struct
	{
		uint64_t fault_generation = 0;
		uint64_t cnf_generation = 0;
		uint64_t cnf_solving = 0;
		uint64_t worst_solving = 0;
	} timing;

	std::unique_ptr<SatSolver> solver = SolverFactory::make_solver();

	if (!solver) {
		log_error() << "No SAT solver, can't run";
		return 1;
	}

	FaultCnfMaker fault_cnf_maker(graph);

	size_t sat = 0;
	size_t unsat = 0;
	size_t unknown = 0;
	size_t total_faults = 0;

	ElapsedTimer total_timer(true);

	ElapsedTimer t(true);
	FaultManager fault_manager(graph);
	timing.fault_generation = t.get_elapsed_us();

	fault_cnf_maker.set_threshold_ratio(g_config.threshold_ratio);

	ProxyCnf proxy(*solver);

	while (fault_manager.has_faults_left()) {
		Fault f = fault_manager.next_fault();
		++total_faults;
		if (g_config.total_time_limit_s && total_timer.get_elapsed_ms() > g_config.total_time_limit_s * 1000) {
			++unknown;
			continue;
		}

		if (g_config.write_faults) {
			if (f.is_stem || f.is_primary_output) {
				log_info() << log_nospace << f.line->name << log_noendl;
				log_info() << "/O" << log_noendl;
			} else {
				const Gate* gate = f.connection.gate;
				auto it = std::find(gate->get_inputs().cbegin(), gate->get_inputs().cend(), f.line);
				if (it == gate->get_inputs().cend()) {
					log_error() << "Broken fault";
					assert(false);
					return 1;
				}
				size_t idx = f.connection.input_idx;
				log_info() << log_nospace << gate->get_output()->name << log_noendl;
				log_info() << log_nospace << "/I" << idx+1 << " " << log_noendl;
			}
			auto logger = log_info();
			logger << log_nospace << "S-A-" << (int)f.stuck_at;
			if (g_config.write_detectability)
				logger << log_noendl;
		}

		t.start();
		fault_cnf_maker.make_fault(f, proxy);
		timing.cnf_generation += t.get_elapsed_us();

		if (g_config.do_solve) {
			t.start();
			SatSolver::SolveStatus status = solver->solve_prepared();
			timing.cnf_solving += t.get_elapsed_us();

			if (t.get_elapsed_us() > timing.worst_solving) {
				timing.worst_solving = t.get_elapsed_us();
			}

			if (g_config.write_solutions) {
				for (Line* l : graph.get_inputs()) {
					int8_t val = solver->get_value(line_to_literal(l->id));
					log_info() << "\t" << l->name << (val == -1 ? 0 : 1);
				}
			}

			if (g_config.write_detectability) {
				log_info() << (status == SatSolver::Sat ? "===DETECTABLE===" : "===REDUNDANT====");
			}

			if (status == SatSolver::Sat) {
				sat += 1;
			} else if (status == SatSolver::Unsat) {
				unsat += 1;
			} else {
				unknown += 1;
			}
		}
	}

	if (g_config.write_stats) {
		if (g_config.short_stats) {
			log_info() << "time (total/gen/solve):" << total_timer.get_elapsed_ms() << timing.cnf_generation/1000 << timing.cnf_solving/1000 << "faults (total/undetectable):" << total_faults << unsat;
		} else {
			log_info() << "Timing:";
			log_info() << "  " << "Fault generation:" << timing.fault_generation/1000 << "ms";
			log_info() << "  " << "CNF generation:" << timing.cnf_generation/1000 << "ms";
			log_info() << "  " << "CNF solving:" << timing.cnf_solving/1000 << "ms";
			log_info() << "  " << "Slowest solve time:" << timing.worst_solving/1000 << "ms";
			log_info() << "  " << "Total:" << total_timer.get_elapsed_ms() << "ms";
			log_info() << "";

			log_info() << "Total:" << total_faults;
			log_info() << "Detectable:" << sat;
			log_info() << "Undetectable:" << unsat;
			log_info() << "UNKNOWN:" << unknown;
		}
	}

	return 0;
}

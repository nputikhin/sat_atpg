#include "sat_solver.h"

#if USE_CADICAL

#include "cadical.h"

namespace SolverFactory
{
	std::unique_ptr<SatSolver> make_solver()
	{
		return std::unique_ptr<SatSolver>(new CadicalSolver());
	}
}

#else

namespace SolverFactory
{
	std::unique_ptr<SatSolver> make_solver()
	{
		return nullptr;
	}
}

#endif


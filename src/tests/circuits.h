#include "../circuit_graph.h"
#include "../iscas89_parser.h"

#include <catch.hpp>

#include <sstream>

struct C17Circuit
{
	C17Circuit()
	{
		std::string c17_str = R"r(
			# c17
			# 5 inputs
			# 2 outputs
			# 0 inverter
			# 6 gates ( 6 NANDs )
			INPUT(1)
			INPUT(2)
			INPUT(3)
			INPUT(6)
			INPUT(7)

			OUTPUT(22)
			OUTPUT(23)

			10 = NAND(1, 3)
			11 = NAND(3, 6)
			16 = NAND(2, 11)
			19 = NAND(11, 7)
			22 = NAND(10, 16)
			23 = NAND(16, 19)
		)r";

		Iscas89Parser parser;

		std::stringstream c17_ss(c17_str);
		REQUIRE(parser.parse(c17_ss, graph));

		l1 = graph.get_line("1");
		l2 = graph.get_line("2");
		l3 = graph.get_line("3");
		l6 = graph.get_line("6");
		l7 = graph.get_line("7");

		l22 = graph.get_line("22");
		l23 = graph.get_line("23");

		l10 = graph.get_line("10");
		l11 = graph.get_line("11");
		l16 = graph.get_line("16");
		l19 = graph.get_line("19");

		g10 = l10->source;
		g11 = l11->source;
		g16 = l16->source;
		g19 = l19->source;
		g22 = l22->source;
		g23 = l23->source;

		REQUIRE(l1);
		REQUIRE(l2);
		REQUIRE(l3);
		REQUIRE(l6);
		REQUIRE(l7);

		REQUIRE(l22);
		REQUIRE(l23);

		REQUIRE(l10);
		REQUIRE(l11);
		REQUIRE(l16);
		REQUIRE(l19);

		REQUIRE(g10);
		REQUIRE(g11);
		REQUIRE(g16);
		REQUIRE(g19);
		REQUIRE(g22);
		REQUIRE(g23);
	}

	CircuitGraph graph;
	Line* l1 = nullptr;
	Line* l2 = nullptr;
	Line* l3 = nullptr;
	Line* l6 = nullptr;
	Line* l7 = nullptr;

	Line* l22 = nullptr;
	Line* l23 = nullptr;

	Line* l10 = nullptr;
	Line* l11 = nullptr;
	Line* l16 = nullptr;
	Line* l19 = nullptr;

	Gate* g10 = nullptr;
	Gate* g11 = nullptr;
	Gate* g16 = nullptr;
	Gate* g19 = nullptr;
	Gate* g22 = nullptr;
	Gate* g23 = nullptr;
};

struct NandXorCircuit
{
	NandXorCircuit()
	{
		std::string str = R"r(
			INPUT(1)
			INPUT(2)

			OUTPUT(3)

			4 = NAND(1, 2)
			5 = NAND(1, 4)
			6 = NAND(2, 4)
			3 = NAND(5, 6)
		)r";

		Iscas89Parser parser;

		std::stringstream ss(str);
		REQUIRE(parser.parse(ss, graph));

		l1 = graph.get_line("1");
		l2 = graph.get_line("2");
		l3 = graph.get_line("3");
		l4 = graph.get_line("4");
		l5 = graph.get_line("5");
		l6 = graph.get_line("6");

		g3 = l3->source;
		g4 = l4->source;
		g5 = l5->source;
		g6 = l6->source;

		REQUIRE(l1);
		REQUIRE(l2);
		REQUIRE(l3);
		REQUIRE(l4);
		REQUIRE(l5);
		REQUIRE(l6);

		REQUIRE(g3);
		REQUIRE(g4);
		REQUIRE(g5);
		REQUIRE(g6);
	}

	CircuitGraph graph;
	Line* l1 = nullptr;
	Line* l2 = nullptr;
	Line* l3 = nullptr;
	Line* l4 = nullptr;
	Line* l5 = nullptr;
	Line* l6 = nullptr;

	Gate* g3 = nullptr;
	Gate* g4 = nullptr;
	Gate* g5 = nullptr;
	Gate* g6 = nullptr;
};

struct S27Circuit
{
	S27Circuit()
	{
		std::string str = R"r(
			# s27
			# 7 inputs
			# 4 outputs
			# 2 inverters
			# 8 gates ( 1 AND + 1 NAND + 2 ORs + 4 NORs + 1 BUFF )

			INPUT(G0)
			INPUT(G1)
			INPUT(G2)
			INPUT(G3)
			INPUT(G5)
			INPUT(G6)
			INPUT(G7)

			OUTPUT(G17)
			OUTPUT(G10)
			OUTPUT(G11_EXTRA)
			OUTPUT(G13)

			G14 = NOT(G0)
			G17 = NOT(G11)
			G8 = AND(G14, G6)
			G15 = OR(G12, G8)
			G16 = OR(G3, G8)
			G9 = NAND(G16, G15)
			G10 = NOR(G14, G11)
			G11 = NOR(G5, G9)
			G12 = NOR(G1, G7)
			G13 = NOR(G2, G12)
			G11_EXTRA = BUFF(G11)
		)r";

		Iscas89Parser parser;

		std::stringstream ss(str);
		REQUIRE(parser.parse(ss, graph));

		l0       	= graph.get_line("G0");
		l1       	= graph.get_line("G1");
		l2       	= graph.get_line("G2");
		l3       	= graph.get_line("G3");
		l5       	= graph.get_line("G5");
		l6       	= graph.get_line("G6");
		l7       	= graph.get_line("G7");
		l14      	= graph.get_line("G14");
		l17      	= graph.get_line("G17");
		l8       	= graph.get_line("G8");
		l15      	= graph.get_line("G15");
		l16      	= graph.get_line("G16");
		l9       	= graph.get_line("G9");
		l10      	= graph.get_line("G10");
		l11      	= graph.get_line("G11");
		l12      	= graph.get_line("G12");
		l13      	= graph.get_line("G13");
		l11_extra	= graph.get_line("G11_EXTRA");

		g14      	= l14->source;
		g17      	= l17->source;
		g8       	= l8->source;
		g15      	= l15->source;
		g16      	= l16->source;
		g9       	= l9->source;
		g10      	= l10->source;
		g11      	= l11->source;
		g12      	= l12->source;
		g13      	= l13->source;
		g11_extra	= l11_extra->source;
	}

	CircuitGraph graph;

	Line* l0;
	Line* l1;
	Line* l2;
	Line* l3;
	Line* l5;
	Line* l6;
	Line* l7;
	Line* l14;
	Line* l17;
	Line* l8;
	Line* l15;
	Line* l16;
	Line* l9;
	Line* l10;
	Line* l11;
	Line* l12;
	Line* l13;
	Line* l11_extra;

	Gate* g14;
	Gate* g17;
	Gate* g8;
	Gate* g15;
	Gate* g16;
	Gate* g9;
	Gate* g10;
	Gate* g11;
	Gate* g12;
	Gate* g13;
	Gate* g11_extra;
};

struct TestCircuit
{
	// Truth table for valid output and faults:
	// x     000 001 010 011 100 101 110 111
	// y      0   1   0   0   1   1   1   1
	// =====================================
	// gI1@0  0   1   0   0   0   1  *0* *0*
	// gI1@1  0   1  *1* *1*  0   1   1   1
	// x2@0   0   1   0  *1*  0   1  *0*  1
	// x2@1   0  *0*  0   0  *1*  1   1   1
	// hI2@0  0  *0*  0   0   0  *0*  1   1
	// hI2@1 *1*  1   0   0  *1*  1   1   1
	// gI2@0  0   1   0   0   0   1  *0* *0*
	// gI2@1  0   1   0   0  *1*  1   1   1
	// fI1@0  0   1   0  *1*  0   1   1   1
	// fI1@1  0  *0*  0   0   0  *0*  1   1
	// f@0    0  *0*  0   0   0  *0*  1   1
	// f@1    0   1   0  *1*  0   1   1   1
	// g@0    0   1   0   0   0   1  *0* *0*
	// g@1   *1*  1  *1* *1* *1*  1   1   1
	// h@0    0  *0*  0   0   0  *0*  1   1
	// h@1   *1*  1  *1* *1* *1*  1   1   1
	// y@0    0  *0*  0   0   0  *0* *0* *0*
	// y@1   *1*  1  *1* *1* *1*  1   1   1
	TestCircuit()
	{
		//INPUT(x1)
		//INPUT(x2)
		//INPUT(x3)
		//
		//OUTPUT(y)
		//
		//g = AND(x1, x2)
		//f = NOT(x2)
		//h = AND(f, x3)
		//y = OR(g, h)

		graph.add_input("x1");
		graph.add_input("x2");
		graph.add_input("x3");

		graph.add_output("y");

		graph.add_gate(Gate::Type::And, {"x1", "x2"}, "g");
		graph.add_gate(Gate::Type::Not, {"x2"}, "f");
		graph.add_gate(Gate::Type::And, {"f", "x3"}, "h");
		graph.add_gate(Gate::Type::Or, {"g", "h"}, "y");

		x1 = graph.get_line("x1");
		x2 = graph.get_line("x2");
		x3 = graph.get_line("x3");
		f = graph.get_line("f");
		g = graph.get_line("g");
		h = graph.get_line("h");
		y = graph.get_line("y");
	}

	CircuitGraph graph;

	Line* x1;
	Line* x2;
	Line* x3;
	Line* f;
	Line* g;
	Line* h;
	Line* y;
};

struct TestCircuitWithExpandableGates
{
	TestCircuitWithExpandableGates()
	{
		std::string str = R"r(
			INPUT(a)
			INPUT(b)
			INPUT(c)
			INPUT(d)

			OUTPUT(y)

			y = NAND(g1, g13)

			g1  = NAND(g2, g3, g4)
			g2  = NAND(g10, b)
			g3  = NAND(c, g15, g7)
			g4  = NAND(g8, g9)
			g7  = NOT(b)
			g8  = NAND(g11, g12)
			g9  = NOT(c)
			g10 = AND(c, a)
			g11 = NAND(a, g7)
			g12 = NAND(b, g15)
			g13 = NAND(g17, d, g14)
			g14 = NAND(g15, g16)
			g15 = NOT(a)
			g16 = NAND(c, b)

			g17 = OR(c, b)
		)r";

		Iscas89Parser parser;

		std::stringstream ss(str);
		REQUIRE(parser.parse(ss, graph));

		a  = graph.get_line("a");
		b  = graph.get_line("b");
		c  = graph.get_line("c");
		d  = graph.get_line("d");

		y   = graph.get_line("y");
		g1  = graph.get_line("g1");
		g2  = graph.get_line("g2");
		g3  = graph.get_line("g3");
		g4  = graph.get_line("g4");
		g7  = graph.get_line("g7");
		g8  = graph.get_line("g8");
		g9  = graph.get_line("g9");
		g10 = graph.get_line("g10");
		g11 = graph.get_line("g11");
		g12 = graph.get_line("g12");
		g13 = graph.get_line("g13");
		g14 = graph.get_line("g14");
		g15 = graph.get_line("g15");
		g16 = graph.get_line("g16");
		g17 = graph.get_line("g17");
	}

	CircuitGraph graph;

	Line* a;
	Line* b;
	Line* c;
	Line* d;

	Line* y;
	Line* g1;
	Line* g2;
	Line* g3;
	Line* g4;
	Line* g7;
	Line* g8;
	Line* g9;
	Line* g10;
	Line* g11;
	Line* g12;
	Line* g13;
	Line* g14;
	Line* g15;
	Line* g16;
	Line* g17;
};

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <dballe/err/dba_error.h>

#include <sys/times.h>

#include <vector>
#include <string>

class Benchmark
{
private:
	static double tps;
	std::string tag;
	struct tms lasttms;
	Benchmark* parent;
	std::vector<Benchmark*> children;
	
protected:
	// Main function with the benchmarks
	virtual dba_err main() { return dba_error_ok(); }

	dba_err timing(const char* fmt, ...);

	void setParent(Benchmark* parent) { this->parent = parent; }

	std::string name() const { return tag; }

	std::string fullName() const
	{
		if (parent)
			return parent->fullName() + "/" + tag;
		else
			return tag;
	}
	
public:
	Benchmark(const std::string& tag) : tag(tag), parent(0) {}

	void addChild(Benchmark* child)
	{
		children.push_back(child);
		child->setParent(this);
	}

	virtual ~Benchmark()
	{
		for (std::vector<Benchmark*>::iterator i = children.begin();
				i != children.end(); i++)
			delete *i;
	}

	// Run only the subtest at the given path
	dba_err run(const std::string& path);
	
	// Run all subtests and this test
	dba_err run();

	// Return the singleton instance of the toplevel benchmark class
	static Benchmark* root();
};

struct RegisterRoot
{
	RegisterRoot(Benchmark* b)
	{
		Benchmark::root()->addChild(b);
	}
};

// vim:set ts=4 sw=4:
#endif

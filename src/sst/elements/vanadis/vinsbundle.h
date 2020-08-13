
#ifndef _H_VANADIS_INST_BUNDLE
#define _H_VANADIS_INST_BUNDLE

#include <vector>
#include <cstdint>
#include <cinttypes>

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisInstructionBundle {

public:
	VanadisInstructionBundle( const uint64_t addr ) : ins_addr(addr) {
		inst_bundle.reserve(1);
	}

	~VanadisInstructionBundle() {
		for( VanadisInstruction* next_ins : inst_bundle ) {
			delete next_ins;
		}

		inst_bundle.clear();
	}

	uint32_t getInstructionCount() const {
		return inst_bundle.size();
	}

	void addInstruction( VanadisInstruction* newIns ) {
		inst_bundle.push_back(newIns->clone() );
	}

	VanadisInstruction* getInstructionByIndex( const uint32_t index ) {
		return inst_bundle[index]->clone();
	}

	VanadisInstruction* getInstructionByIndex( const uint32_t index, const uint64_t new_id ) {
		VanadisInstruction* new_ins = inst_bundle[index]->clone();
		new_ins->setID( new_id );
		return new_ins;
	}

	uint64_t getInstructionAddress() const {
		return ins_addr;
	}

	VanadisInstructionBundle* clone() {
		VanadisInstructionBundle* new_bundle = new VanadisInstructionBundle( ins_addr );

		for( VanadisInstruction* next_ins : inst_bundle ) {
			new_bundle->addInstruction( next_ins->clone() );
		}

		return new_bundle;
	}

	VanadisInstructionBundle* clone( const uint64_t base_ins_id ) {
		VanadisInstructionBundle* new_bundle = new VanadisInstructionBundle( ins_addr );
		uint64_t next_id = base_ins_id;

		for( VanadisInstruction* next_ins : inst_bundle ) {
			VanadisInstruction* cloned_ins = next_ins->clone();
			cloned_ins->setID( next_id++ );
			new_bundle->addInstruction( cloned_ins );
		}

		return new_bundle;
	}

private:
	const uint64_t ins_addr;
	std::vector<VanadisInstruction*> inst_bundle;
};

}
}

#endif

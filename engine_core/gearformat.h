#include "globals.h"

class gearFormat {
	public:
		unsigned int version;
		// game assets <assets path - asset data>
		std::map<std::string, std::vector<char>> assets;

		template <class Archive> void serialize(Archive& archive) { archive(version, assets); }
};

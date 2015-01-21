#include "grid.h"

namespace Eigen {
		size_t hash_value(Vector3f const &v) {
			size_t seed = 0;
			for (int i=0;i<3;i++) boost::hash_combine(seed, v[i]);
			return seed;
		}
		size_t hash_value(Vector3d const &v) {
			size_t seed = 0;
			for (int i=0;i<3;i++) boost::hash_combine(seed, v[i]);
			return seed;
		}
		size_t hash_value(Eigen::Matrix<int64_t, 3, 1> const &v) {
			size_t seed = 0;
			for (int i=0;i<3;i++) boost::hash_combine(seed, v[i]);
			return seed;
		}
}

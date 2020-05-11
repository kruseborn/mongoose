#include "sdf.h"

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <cstring>

Sdf objToSdf(std::string objPath, float dx, uint32_t padding) {
  Vec3f min_box(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()),
      max_box(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
              -std::numeric_limits<float>::max());

  std::ifstream infile(objPath);
  if (!infile) {
    std::cerr << "Failed to open. Terminating.\n";
    exit(-1);
  }

  int ignored_lines = 0;
  std::string line;
  std::vector<Vec3f> vertList;
  std::vector<Vec3ui> faceList;
  while (!infile.eof()) {
    std::getline(infile, line);

    //.obj files sometimes contain vertex normals indicated by "vn"
    if (line.substr(0, 1) == std::string("v") && line.substr(0, 2) != std::string("vn")) {
      std::stringstream data(line);
      char c;
      Vec3f point;
      data >> c >> point[0] >> point[1] >> point[2];
      vertList.push_back(point);
      update_minmax(point, min_box, max_box);
    } else if (line.substr(0, 1) == std::string("f")) {
      std::stringstream data(line);
      char c;
      int v0, v1, v2;
      data >> c >> v0 >> v1 >> v2;
      faceList.push_back(Vec3ui(v0 - 1, v1 - 1, v2 - 1));
    } else if (line.substr(0, 2) == std::string("vn")) {
      std::cerr << "Obj-loader is not able to parse vertex normals, please strip them from the input file. \n";
      exit(-2);
    } else {
      ++ignored_lines;
    }
  }
  infile.close();
  if (ignored_lines > 0)
    std::cout << "Warning: " << ignored_lines << " lines were ignored since they did not contain faces or vertices.\n";

  std::cout << "Read in " << vertList.size() << " vertices and " << faceList.size() << " faces." << std::endl;

  // Add padding around the box.
  Vec3f unit(1, 1, 1);
  min_box -= padding * dx * unit;
  max_box += padding * dx * unit;
  Vec3ui sizes = Vec3ui((max_box - min_box) / dx);

  std::cout << "Bound box size: (" << min_box << ") to (" << max_box << ") with dimensions " << sizes << "."
            << std::endl;

  std::cout << "Computing signed distance field.\n";
  Array3f phi_grid;
  make_level_set3(faceList, vertList, min_box, dx, sizes[0], sizes[1], sizes[2], phi_grid);

  Sdf sdf = {};
  sdf.minBox = min_box;
  sdf.maxBox = max_box;
  sdf.x = phi_grid.ni;
  sdf.y = phi_grid.nj;
  sdf.z = phi_grid.nk;

  sdf.grid.resize(sdf.x * sdf.y * sdf.z);
  std::memcpy(sdf.grid.data(), phi_grid.a.data, sdf.grid.size()*sizeof(float));
  return sdf;
}
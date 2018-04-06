/*
    @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
   /@@/////  /@@          @@////@@ @@////// /@@
   /@@       /@@  @@@@@  @@    // /@@       /@@
   /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@
   /@@////   /@@/@@@@@@@/@@       ////////@@/@@
   /@@       /@@/@@//// //@@    @@       /@@/@@
   /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@
   //       ///  //////   //////  ////////  //

   Copyright (c) 2016, Los Alamos National Security, LLC
   All rights reserved.
                                                                              */

#include <iostream>

#include<flecsi-tutorial/specialization/mesh/mesh.h>
#include<flecsi/data/data.h>
#include<flecsi/execution/execution.h>
#include<flecsi-tutorial/specialization/io/vtk/unstructuredGrid.h>

using namespace flecsi;
using namespace flecsi::tutorial;

flecsi_register_field(mesh_t, example, field, double, dense, 1, cells);

namespace example {

void initialize_field(mesh<ro> mesh, field<rw> f) {
  for(auto c: mesh.cells(owned)) {
    f(c) = double(c->id());
  } // for
} // initialize_field

flecsi_register_task(initialize_field, example, loc, single);

void print_field(mesh<ro> mesh, field<ro> f) {
  for(auto c: mesh.cells(owned)) {
    std::cout << "cell id: " << c->id() << " has value " <<
      f(c) << std::endl;
  } // for
} // print_field

flecsi_register_task(print_field, example, loc, single);



void output_field(mesh<ro> mesh, field<ro> f) 
{
  vtkOutput::UnstructuredGrid temp;
  double *cellData = new double[256];
  double *cellID = new double[256];

  int count = 0;

  for(auto c: mesh.cells(owned)) {

    //std::vector<double> pointCoords;
    double * pointCoords = new double[8];
    int vertexCount = 0;
    for(auto v: mesh.vertices(c)) {
      auto p = v->coordinates();
      double x = std::get<0>(p);
      double y = std::get<1>(p);
     // pointCoords.push_back(x);
      //pointCoords.push_back(y);
      pointCoords[vertexCount*2+0] = x;
      pointCoords[vertexCount*2+1] = y;
      std::cout << x << ", " << y << std::endl;
      vertexCount++;
    }
    
    std::cout << std::endl << std::endl;

    temp.uGrid->InsertNextCell(VTK_QUAD, vertexCount*2, pointCoords);
    
    //double pnt[3];
    //pnt[0]=c->id(); pnt[1]=0; pnt[2]=0;
    //temp.addPoint(pnt);
    cellID[count] = c->id();

    cellData[count] = f(c);
    count++;
  } // for
  //temp.pushPointsToGrid(VTK_VERTEX);

  temp.addScalarData("cell-id", 256, cellID);
  temp.addScalarData("cell-data-scalar", 256, cellData);
  temp.write("testVTK");

  if (cellData != NULL)
   delete []cellData;
  cellData = NULL;
} // print_field


flecsi_register_task(output_field, example, loc, single);

} // namespace example

namespace flecsi {
namespace execution {

void driver(int argc, char ** argv) {

  auto m = flecsi_get_client_handle(mesh_t, clients, mesh);
  auto f = flecsi_get_handle(m, example, field, double, dense, 0);

  flecsi_execute_task(initialize_field, example, single, m, f);
  flecsi_execute_task(print_field, example, single, m, f);
  flecsi_execute_task(output_field, example, single, m, f);
} // driver

} // namespace execution
} // namespace flecsi

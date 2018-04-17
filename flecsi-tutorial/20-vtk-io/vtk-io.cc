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
#include "/home/pascal/projects/NGC/flecsi/flecsi-tutorial/specialization/io/vtk/utils.h"
#include <vtkQuad.h>
#include <map>


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
  vtkOutput::UnstructuredGrid tutorial2dMesh;

  // 
  // Find the list of points and insert them in pnts - All f this to avoid inserting duplicate points
  std::map < size_t, vtkOutput::_point > indexPoint;

  for(auto c: mesh.cells(owned)) {
    for(auto v: mesh.vertices(c)) {
      auto p = v->coordinates();
      indexPoint.insert( std::pair< size_t, vtkOutput::_point > ( v->coordinateID(), vtkOutput::_point( std::get<0>(p), std::get<1>(p) ) ) );
    }
  }

  for (int i=0; i<indexPoint.size(); i++)
    tutorial2dMesh.addVertex( indexPoint[i].coords );


  //
  // Insert mesh info
  std::vector<double> cellData, cellID;

  for(auto c: mesh.cells(owned)) {
    //
    // Insert Cell
    vtkSmartPointer<vtkQuad> quad = vtkSmartPointer<vtkQuad>::New();
    int localVertexCount = 0;
    for(auto v: mesh.vertices(c)) 
    {
      quad->GetPointIds()->SetId(localVertexCount,v->coordinateID()); localVertexCount++;
    }
    tutorial2dMesh.addCell(quad);
    
    // Insert Data
    cellData.push_back( f(c) );
    cellID.push_back( c->id() );
  } // for
  

  tutorial2dMesh.pushTopologyToGrid(VTK_QUAD);
  tutorial2dMesh.addScalarData("cell-id", cellID.size(), &cellID[0], 1);
  tutorial2dMesh.addScalarData("cell-data-scalar", cellData.size(), &cellData[0], 1);
  tutorial2dMesh.write("tutorialMesh");

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

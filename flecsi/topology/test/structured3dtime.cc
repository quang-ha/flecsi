#include <iostream>
#include <sys/time.h>
#include <cinchtest.h>
#include "flecsi/topology/structured_mesh_topology.h"

using namespace std;
using namespace flecsi;
using namespace topology;


double wtime()
{
  double y = -1;
  struct timeval tm;
  gettimeofday(&tm,NULL);
  y = (double)(tm.tv_sec)+(double)(tm.tv_usec)*1.e-6;
  return y;
};

struct smi_perf
{
  double meshgen;
  double v2v, v2e, v2f, v2c;
  double e2v, e2e, e2f, e2c;
  double f2v, f2e, f2f, f2c;
  double c2v, c2e, c2f, c2c;
};

class Vertex : public structured_mesh_entity_t<0, 1>{
public:
  Vertex(){}

  Vertex(structured_mesh_topology_base_t &){}
};

class Edge : public structured_mesh_entity_t<1, 1>{
public:
  Edge(){}

  Edge(structured_mesh_topology_base_t &){}
};

class Face : public structured_mesh_entity_t<2, 1>{
public:

  Face(){}

  Face(structured_mesh_topology_base_t &){}
};

class Cell : public structured_mesh_entity_t<3, 1>{
public:

  Cell(){}

  Cell(structured_mesh_topology_base_t &){}
};


class TestMesh3dType{
public:
  static constexpr size_t num_dimensions = 3;
  static constexpr size_t num_domains = 1;

  static constexpr std::array<size_t,num_dimensions> lower_bounds = {0,0,0};
  static constexpr std::array<size_t,num_dimensions> upper_bounds = {9,9,9};

  using entity_types = std::tuple<
  std::pair<domain_<0>, Vertex>,
  std::pair<domain_<0>, Edge>,
  std::pair<domain_<0>, Face>,
  std::pair<domain_<0>, Cell>>;

};

constexpr std::array<size_t,TestMesh3dType::num_dimensions> TestMesh3dType::lower_bounds;
constexpr std::array<size_t,TestMesh3dType::num_dimensions> TestMesh3dType::upper_bounds;

using id_vector_t = std::vector<size_t>;
using TestMesh = structured_mesh_topology_t<TestMesh3dType>;

TEST(structured3dtime, simple){
  
  smi_perf *sp = new smi_perf;
  double time_start, time_end;

  time_start = wtime();
  auto mesh = new TestMesh;
  sp->meshgen = wtime() - time_start;
 
  size_t nv, ne, nf, nc, val;

  auto lbnd = mesh->lower_bounds();
  auto ubnd = mesh->upper_bounds();

  CINCH_CAPTURE() << "3D Logically structured mesh with bounds: [" <<lbnd[0]<<
  ", "<<lbnd[1]<<", "<<lbnd[2]<<"] - ["<<ubnd[0]<<", "<<ubnd[1]<<", "<<ubnd[2]<<"] \n"<< endl;

  
  nv  = mesh->num_entities(0,0);
  ne  = mesh->num_entities(1,0);
  nf  = mesh->num_entities(2,0);
  nc  = mesh->num_entities(3,0);
  
  CINCH_CAPTURE()<<"|V| = "<<nv<<", |E| = "<<ne<<", |F| = "<<nf<<", |C| = "<<nc<<endl;
  
  //Loop over all vertices and test intra index space queries
  //V-->V
  time_start = wtime();
  for (auto vertex: mesh->entities<0>()){
    val = mesh->stencil_entity< 1, 0, 0, 0 >(&vertex);
    val = mesh->stencil_entity<-1, 0, 0, 0 >(&vertex); 
    val = mesh->stencil_entity< 0, 1, 0, 0 >(&vertex);
    val = mesh->stencil_entity< 0,-1, 0, 0 >(&vertex);
    val = mesh->stencil_entity< 0, 0, 1, 0 >(&vertex);
    val = mesh->stencil_entity< 0, 0,-1, 0 >(&vertex);
  }
  sp->v2v = (wtime()-time_start)/(double)(6*nv);
 
  //V-->E
  time_start = wtime();
  for (auto vertex: mesh->entities<0>())
  {
    size_t count = 0;
    for (auto edge: mesh->entities<1,0>(&vertex))
    {++count;};
  }
  sp->v2e = (wtime()-time_start)/(double)(nv);
   
  //V-->F
  time_start = wtime();
  for (auto vertex: mesh->entities<0>())
  {
    size_t count = 0;
    for (auto face: mesh->entities<2,0>(&vertex))
    {++count;};
  }
  sp->v2f = (wtime()-time_start)/(double)(nv);

  //V-->C
  time_start = wtime();
  for (auto vertex: mesh->entities<0>())
  {
    size_t count = 0; 
    for (auto cell: mesh->entities<3,0>(&vertex))
    {++count;};
  }
  sp->v2c = (wtime()-time_start)/(double)(nv);
  
  
  //Loop over all edges in X-direction and test intra index space queries
  //E-->V
  time_start = wtime();
  for (auto edge: mesh->entities<1>())
   {
     size_t count = 0;
     for (auto vertex: mesh->entities<0,0>(&edge))
     {++count;};
   }
  sp->e2v = (wtime()-time_start)/(double)(ne); 
  
  //E-->E
  time_start = wtime();
  for (auto edge: mesh->entities<1>())
  {
    val = mesh->stencil_entity< 1, 0, 0, 0 >(&edge);
    val = mesh->stencil_entity<-1, 0, 0, 0 >(&edge); 
    val = mesh->stencil_entity< 0, 1, 0, 0 >(&edge);
    val = mesh->stencil_entity< 0,-1, 0, 0 >(&edge);
    val = mesh->stencil_entity< 0, 0, 1, 0 >(&edge);
    val = mesh->stencil_entity< 0, 0,-1, 0 >(&edge);
  }
  sp->e2e = (wtime()-time_start)/(double)(6*(ne));

  //E-->F
  time_start = wtime();
  for (auto edge: mesh->entities<1>())
   {
     size_t count = 0;
     for (auto face: mesh->entities<2,0>(&edge))
     {++count;};
   }
   sp->e2f = (wtime()-time_start)/(double)(ne);

  //E-->C
   time_start = wtime();
  for (auto edge: mesh->entities<1>())
   {
     size_t count = 0;
     for (auto cell: mesh->entities<3,0>(&edge))
     {++count;};
   }
   sp->e2c = (wtime()-time_start)/(double)(ne);
  
  //F-->V
   time_start = wtime();
  for (auto face: mesh->entities<2>())
   {
     size_t count = 0;
     for (auto vertex: mesh->entities<0,0>(&face))
     {++count;};
   }
   sp->f2v = (wtime()-time_start)/(double)(nf);

  //F-->E
  time_start = wtime();
  for (auto face: mesh->entities<2>())
   {
     size_t count = 0;
     for (auto edge: mesh->entities<1,0>(&face))
     {++count;};
   }
   sp->f2e = (wtime()-time_start)/(double)(nf);

   //F-->F
   time_start = wtime();
  for (auto face: mesh->entities<2>())
  {
    val = mesh->stencil_entity< 1, 0, 0, 0 >(&face);
    val = mesh->stencil_entity<-1, 0, 0, 0 >(&face); 
    val = mesh->stencil_entity< 0, 1, 0, 0 >(&face);
    val = mesh->stencil_entity< 0,-1, 0, 0 >(&face);
    val = mesh->stencil_entity< 0, 0, 1, 0 >(&face);
    val = mesh->stencil_entity< 0, 0,-1, 0 >(&face);
  }
  sp->f2f = (wtime()-time_start)/(double)(6*(nf));

  //F-->C
  time_start = wtime();
  for (auto face: mesh->entities<2>())
   {
     size_t count = 0;
     for (auto cell: mesh->entities<3,0>(&face))
     {++count;};
   }
   sp->f2c = (wtime()-time_start)/(double)(nf);

  //C-->V
   time_start = wtime();
  for (auto cell: mesh->entities<3>())
   {
     size_t count = 0;
     for (auto vertex: mesh->entities<0,0>(&cell))
     {++count;};
   }
   sp->c2v = (wtime()-time_start)/(double)(nc);

  //C-->E
   time_start = wtime();
  for (auto cell: mesh->entities<3>())
   {
     size_t count = 0;
     for (auto edge: mesh->entities<1,0>(&cell))
     {++count;};
   }
   sp->c2e = (wtime()-time_start)/(double)(nc);

  //C-->F
   time_start = wtime();
  for (auto cell: mesh->entities<3>())
   {
     size_t count = 0;
     for (auto face: mesh->entities<2,0>(&cell))
     {++count;};
   }
   sp->c2f = (wtime()-time_start)/(double)(nc);
 
  //C-->C
   time_start = wtime();
  for (auto cell: mesh->entities<3>())
  {
    val = mesh->stencil_entity< 1, 0, 0, 0 >(&cell);
    val = mesh->stencil_entity<-1, 0, 0, 0 >(&cell); 
    val = mesh->stencil_entity< 0, 1, 0, 0 >(&cell);
    val = mesh->stencil_entity< 0,-1, 0, 0 >(&cell);
    val = mesh->stencil_entity< 0, 0, 1, 0 >(&cell);
    val = mesh->stencil_entity< 0, 0,-1, 0 >(&cell);
  }
  sp->c2c = (wtime()-time_start)/(double)(6*nc);

  CINCH_CAPTURE()<<"Average times in seconds for mesh traversal\n"<<endl;
  CINCH_CAPTURE()<<"Generating structured mesh :: "<<sp->meshgen<<endl;
  CINCH_CAPTURE()<<endl;
  CINCH_CAPTURE()<<"QUERY: Vertex --> Vertex :: "<<sp->v2v<<endl;
  CINCH_CAPTURE()<<"QUERY: Vertex --> Edge :: "<<sp->v2e<<endl;
  CINCH_CAPTURE()<<"QUERY: Vertex --> Face :: "<<sp->v2f<<endl;
  CINCH_CAPTURE()<<"QUERY: Vertex --> Cell :: "<<sp->v2c<<endl;
  CINCH_CAPTURE()<<endl;
  CINCH_CAPTURE()<<"QUERY: Edge --> Vertex :: "<<sp->e2v<<endl;
  CINCH_CAPTURE()<<"QUERY: Edge --> Edge :: "<<sp->e2e<<endl;
  CINCH_CAPTURE()<<"QUERY: Edge --> Face :: "<<sp->e2f<<endl;
  CINCH_CAPTURE()<<"QUERY: Edge --> Cell :: "<<sp->e2c<<endl;
  CINCH_CAPTURE()<<endl;
  CINCH_CAPTURE()<<"QUERY: Face --> Vertex :: "<<sp->f2v<<endl;
  CINCH_CAPTURE()<<"QUERY: Face --> Edge :: "<<sp->f2e<<endl;
  CINCH_CAPTURE()<<"QUERY: Face --> Face :: "<<sp->f2f<<endl;
  CINCH_CAPTURE()<<"QUERY: Face --> Cell :: "<<sp->f2c<<endl;
  CINCH_CAPTURE()<<endl;
  CINCH_CAPTURE()<<"QUERY: Cell --> Vertex :: "<<sp->c2v<<endl;
  CINCH_CAPTURE()<<"QUERY: Cell --> Edge :: "<<sp->c2e<<endl;
  CINCH_CAPTURE()<<"QUERY: Cell --> Face :: "<<sp->c2f<<endl;
  CINCH_CAPTURE()<<"QUERY: Cell --> Cell :: "<<sp->c2c<<endl;

  CINCH_WRITE("structured3dtime.blessed");

  delete mesh;
} // TEST

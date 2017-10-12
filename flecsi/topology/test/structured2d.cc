#include <iostream>
#include <cinchtest.h>
#include "flecsi/topology/structured_mesh_topology.h"

using namespace std;
using namespace flecsi;
using namespace topology;


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


class TestMesh2dType{
public:
  static constexpr size_t num_dimensions = 2;
  static constexpr size_t num_domains = 1;

  static constexpr std::array<size_t,num_dimensions> lower_bounds = {0,0};
  static constexpr std::array<size_t,num_dimensions> upper_bounds = {1,1};

  using entity_types = std::tuple<
  std::pair<domain_<0>, Vertex>,
  std::pair<domain_<0>, Edge>,
  std::pair<domain_<0>, Face>>;

};

constexpr std::array<size_t,TestMesh2dType::num_dimensions> TestMesh2dType::lower_bounds;
constexpr std::array<size_t,TestMesh2dType::num_dimensions> TestMesh2dType::upper_bounds;

using id_vector_t = std::vector<size_t>;
using TestMesh = structured_mesh_topology_t<TestMesh2dType>;

TEST(structured, simple){

  auto mesh = new TestMesh; 
  size_t nv, ne, nf;
  id_vector_t adj;

  auto lbnd = mesh->lower_bounds();
  auto ubnd = mesh->upper_bounds();

  CINCH_CAPTURE() << "2D Logically structured mesh with bounds: [" <<lbnd[0]<<
  ", "<<lbnd[1]<<"] - ["<<ubnd[0]<<", "<<ubnd[1]<<"] \n"<< endl;

  nv = mesh->num_entities(0,0);
  ne = mesh->num_entities(1,0);
  nf = mesh->num_entities(2,0);
  
  CINCH_CAPTURE() << "NV = " << nv << endl;
  CINCH_CAPTURE() << "NE = " << ne << endl;
  CINCH_CAPTURE() << "NF = " << nf << endl;
  CINCH_CAPTURE()<<endl;
 
  //Loop over all vertices and test intra index space queries
  CINCH_CAPTURE()<<"------Vertices------"<<endl;
  for (auto vertex: mesh->entities<0>()){
   std::cout<<" vid = "<<vertex.id(0) <<std::endl;
   CINCH_CAPTURE() << "---- vertex id: " << vertex.id(0) << endl; 
  /* CINCH_CAPTURE() << "  -- indices "<< endl; 
   for (auto idv : mesh->get_indices<0>(vertex))
    CINCH_CAPTURE() << "  ---- " <<idv << endl; 
   auto id = mesh->get_indices<0>(vertex);
   auto offset = mesh->get_offset<0>(id); 
   CINCH_CAPTURE() << "  ---- offset " <<offset<< endl; 
   ASSERT_EQ(vertex,offset); 

   //V-->V
   CINCH_CAPTURE() << "  -- stencil [1 0] " <<mesh->entities<0,0,1,0>(vertex) << endl;
   CINCH_CAPTURE() << "  -- stencil [0 1] " <<mesh->entities<0,0,0,1>(vertex) << endl;
   CINCH_CAPTURE() << "  -- stencil [-1 0] " <<mesh->entities<0,0,-1,0>(vertex) << endl; 
   CINCH_CAPTURE() << "  -- stencil [0 -1] " <<mesh->entities<0,0,0,-1>(vertex) << endl;
  */
   //V-->E
   CINCH_CAPTURE() << "  -- query V-->E "<< endl; 
   for (auto edge : mesh->entities<1,0>(&vertex)){
    std::cout<<"  ---- eid = "<<edge.id(0) <<std::endl;
    CINCH_CAPTURE() << "  ---- " <<edge.id(0)<< endl;} 
  /*
   //V-->F
   CINCH_CAPTURE() << "  -- query V-->F "<< endl; 
   for (auto face : mesh->get_entities<0,2,0>(vertex))
    CINCH_CAPTURE() << "  ---- " <<face<< endl; 
  
   CINCH_CAPTURE()<<endl;*/
  }
  
  //Loop over all edges in X-direction and test intra index space queries
  /*CINCH_CAPTURE()<<"------Edges------"<<endl;
  for (auto edge: mesh->entities<1>()){
   CINCH_CAPTURE() << "---- edge id: " << edge.id() << endl;
   /*CINCH_CAPTURE() << "  -- indices "<< endl; 
   for (auto idv : mesh->get_indices<1>(edgex))
    CINCH_CAPTURE() << "  ---- " <<idv << endl; 
   auto id = mesh->get_indices<1>(edgex); 
   auto offset = mesh->get_offset<1>(id); 
   CINCH_CAPTURE() << "  ---- offset " <<offset<< endl; 
   ASSERT_EQ(edgex,offset); 
    

   //E-->E
   CINCH_CAPTURE() << "  -- stencil [1 0] " <<mesh->entities<1,0,1,0>(edgex) << endl;
   CINCH_CAPTURE() << "  -- stencil [0 1] " <<mesh->entities<1,0,0,1>(edgex) << endl;
   CINCH_CAPTURE() << "  -- stencil [-1 0] " <<mesh->entities<1,0,-1,0>(edgex) << endl; 
   CINCH_CAPTURE() << "  -- stencil [0 -1] " <<mesh->entities<1,0,0,-1>(edgex) << endl;
   
   //E-->V
   CINCH_CAPTURE() << "  -- query E-->V "<< endl; 
   for (auto vert : mesh->get_entities<1,0,0>(edgex))
    CINCH_CAPTURE() << "  ---- " <<vert<< endl; 
   
   //E-->F
   CINCH_CAPTURE() << "  -- query E-->F "<< endl; 
   for (auto face : mesh->get_entities<1,2,0>(edgex))
    CINCH_CAPTURE() << "  ---- " <<face<< endl; 
   CINCH_CAPTURE()<<endl;
  }*/


  //Loop over all faces and test intra index space queries
 /* CINCH_CAPTURE()<<"------Faces------"<<endl;
  for (auto face: mesh->entities<2>()){
   CINCH_CAPTURE() << "---- face id: " << face.id() << endl;
  *CINCH_CAPTURE() << "  -- indices "<< endl; 
   for (auto idv : mesh->get_indices<3>(face))
    CINCH_CAPTURE() << "  ---- " <<idv << endl; 
   auto id = mesh->get_indices<3>(face); 
   auto offset = mesh->get_offset<3>(id); 
   CINCH_CAPTURE() << "  ---- offset " <<offset<< endl; 
   ASSERT_EQ(face,offset); 
    
  
   //F-->F
   CINCH_CAPTURE() << "  -- stencil [1 0] " <<mesh->entities<2,0,1,0>(face) << endl;
   CINCH_CAPTURE() << "  -- stencil [0 1] " <<mesh->entities<2,0,0,1>(face) << endl;
   CINCH_CAPTURE() << "  -- stencil [-1 0] " <<mesh->entities<2,0,-1,0>(face) << endl; 
   CINCH_CAPTURE() << "  -- stencil [0 -1] " <<mesh->entities<2,0,0,-1>(face) << endl;
   
   //F-->V
   CINCH_CAPTURE() << "  -- query F-->V "<< endl; 
   for (auto vert : mesh->get_entities<2,0,0>(face))
    CINCH_CAPTURE() << "  ---- " <<vert<< endl; 
   
   //F-->E
   CINCH_CAPTURE() << "  -- query F-->E "<< endl; 
   for (auto edg : mesh->get_entities<2,1,0>(face))
    CINCH_CAPTURE() << "  ---- " <<edg<< endl; 
   CINCH_CAPTURE()<<endl;
  }*/
  

  CINCH_WRITE("structured2d.blessed");
  //ASSERT_TRUE(CINCH_EQUAL_BLESSED("structured.blessed"));

} // TEST

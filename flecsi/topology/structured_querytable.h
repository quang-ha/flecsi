#ifndef structured_querytable_h
#define structured_querytable_h

namespace flecsi {
namespace topology{
namespace query{

template<size_t MD>
struct QueryUnit
{
  size_t                        box_id;
  size_t                        numchk;
  std::array<size_t, MD>           dir;
  std::array<size_t, MD>       bnd_chk;
  std::array<std::intmax_t, MD> offset;
};

template<size_t MD>
struct QuerySequence
{
  std::vector<QueryUnit<MD>> adjacencies;
  size_t size(){return adjacencies.size();};
};

template<size_t MD, size_t MAXFD, size_t MAXIN, size_t MAXTD>
struct QueryTable
{
  QuerySequence<MD> entry[MAXFD][MAXIN][MAXTD];
}; 

auto qtable(size_t MD)
{
  if (MD==2) 
    {
      QueryTable<2,3,2,3> *qt2 = new QueryTable<2,3,2,3>();;

      size_t FD = 3, IN = 2, TD = 3;

      for (size_t i=0; i<FD; i++)
       for (size_t j=0; i<IN; i++)
        for (size_t k=0; i<TD; i++)
         qt2->entry[i][j][k] = QuerySequence<2>();

      //V-->E
      QueryUnit<2> qu1 = {1,1,{0,0},{1,0},{0,0}};
      QueryUnit<2> qu2 = {0,1,{1,0},{1,0},{0,0}};
      QueryUnit<2> qu3 = {1,1,{0,0},{0,0},{-1,0}};
      QueryUnit<2> qu4 = {0,1,{1,0},{0,0},{0,-1}};

      qt2->entry[0][0][1].adjacencies.push_back(qu1);
      qt2->entry[0][0][1].adjacencies.push_back(qu2);
      qt2->entry[0][0][1].adjacencies.push_back(qu3);
      qt2->entry[0][0][1].adjacencies.push_back(qu4);

      //V-->F
      QueryUnit<2> qu5 = {0,2,{0,1},{1,1},{0,0}};
      QueryUnit<2> qu6 = {0,2,{0,1},{0,1},{-1,0}};
      QueryUnit<2> qu7 = {0,2,{0,1},{0,0},{-1,-1}};
      QueryUnit<2> qu8 = {0,2,{0,1},{1,0},{0,-1}};

      qt2->entry[0][0][2].adjacencies.push_back(qu5);
      qt2->entry[0][0][2].adjacencies.push_back(qu6);
      qt2->entry[0][0][2].adjacencies.push_back(qu7);
      qt2->entry[0][0][2].adjacencies.push_back(qu8);

      //E-->V
      QueryUnit<2> qu9 = {0,0,{},{},{0,0}};
      QueryUnit<2> qu10 = {0,0,{},{},{0,1}};

      qt2->entry[1][0][0].adjacencies.push_back(qu9);
      qt2->entry[1][0][0].adjacencies.push_back(qu10);
      
      QueryUnit<2> qu11 = {0,0,{},{},{0,0}};
      QueryUnit<2> qu12 = {0,0,{},{},{1,0}};

      qt2->entry[1][1][0].adjacencies.push_back(qu11);
      qt2->entry[1][1][0].adjacencies.push_back(qu12);

      //E-->F
      QueryUnit<2> qu13 = {0,1,{0,0},{1,0},{0,0}};
      QueryUnit<2> qu14 = {0,1,{0,0},{0,0},{-1,0}};

      qt2->entry[1][0][2].adjacencies.push_back(qu13);
      qt2->entry[1][0][2].adjacencies.push_back(qu14);
      
      QueryUnit<2> qu15 = {0,1,{1,0},{1,0},{0,0}};
      QueryUnit<2> qu16 = {0,1,{1,0},{0,0},{0,-1}};

      qt2->entry[1][1][2].adjacencies.push_back(qu15);
      qt2->entry[1][1][2].adjacencies.push_back(qu16);

      //F-->V
      QueryUnit<2> qu17 = {0,0,{},{},{0,0}};
      QueryUnit<2> qu18 = {0,0,{},{},{1,0}};
      QueryUnit<2> qu19 = {0,0,{},{},{1,1}};
      QueryUnit<2> qu20 = {0,0,{},{},{0,1}};

      qt2->entry[2][0][0].adjacencies.push_back(qu17);
      qt2->entry[2][0][0].adjacencies.push_back(qu18);
      qt2->entry[2][0][0].adjacencies.push_back(qu19);
      qt2->entry[2][0][0].adjacencies.push_back(qu20);

      //F-->E
      QueryUnit<2> qu21 = {1,0,{},{},{0,0}};
      QueryUnit<2> qu22 = {0,0,{},{},{1,0}};
      QueryUnit<2> qu23 = {1,0,{},{},{0,1}};
      QueryUnit<2> qu24 = {0,0,{},{},{0,0}};

      qt2->entry[2][0][1].adjacencies.push_back(qu21);
      qt2->entry[2][0][1].adjacencies.push_back(qu22);
      qt2->entry[2][0][1].adjacencies.push_back(qu23);
      qt2->entry[2][0][1].adjacencies.push_back(qu24);

      return qt2;
    } 
  else 
   std::cerr<<"not implemented yet";
}


} //query
} //topology
} //flecsi
#endif

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <fstream>

#include "CLI11.hpp"

#include "drat_parse.h"

using namespace dratparse;

using std::cout;
using std::endl;
using std::string;
using std::vector;

struct Step {
  std::vector<int> lits;
  std::vector<int> hints;
  int idx;
  int width; // number of core clauses added
  int activity;
};

class LratMap : public dratparse::DratParserObserver {
public:


  
  LratMap (int nFormulaClauses, int nSkeletonClauses, int nChunks) {
    this->nFormulaClauses = nFormulaClauses;
    this->nSkeletonClauses = nSkeletonClauses;
    this->nChunks = nChunks;
    lastMapped = 0;
    emptyClause = false;
  }

  void Addition(vector<int>& lits, vector<int>& hints, int idx) override {

    // if first time seeing the idx
    if (idx >= stepIdxMap.size() || stepIdxMap[idx] == 0) {
      add_map (idx, idx);
    }

    cout << idx << " ";
    for(auto literal : lits) cout << literal << " ";
    cout << "0 "; 
    for(auto h : hints) cout << stepIdxMap[h] << " ";
    cout << "0 " << endl; 

    lastMapped = idx;
    if (lits[0] == 0) emptyClause = true;

  }

  void Deletion(vector<int>& clause) override { ;
  
  };

  void Restoration(vector<int>& clause) override { ;
   
  };

  void Comment(const string& comment) override { ;

  }

  void init_map (std::string map_path) {
    std::ifstream ifile(map_path);

    for (int i = 0; i < nFormulaClauses + 1; i++) add_map (i,i);


    int nSkeleton = nFormulaClauses + 1;
    bool lastFound = false;

    int sidx,skel,prevSkel;
    while (ifile >> sidx >> skel) {
      add_map (skel, skel);
      add_map (nSkeleton, skel);
      nSkeleton++;
      if (sidx == 0 && lastFound)
        lastSkeleton.push_back(prevSkel);
      else lastFound = true;
      prevSkel = skel;
    }
    lastSkeleton.push_back(prevSkel);

    // // sanity check
    // int chunk = (int) (nSkeletonClauses / nChunks);
    // int remainder = nSkeletonClauses % nChunks;
    // // remainder = 0;
    // int nKept = chunk * (nChunks - 1) + remainder;

    // if (nKept != (nSkeleton - nFormulaClauses -1)) {
    //   cout << "ERROR: mapped " << nSkeleton - nFormulaClauses -1 << " instead of actual " << nKept << endl;
    //   exit (1);
    // }
  }


  bool early_conflict (int i) {
    // cout << lastMapped << " " << lastSkeleton[i] << endl;
    return (emptyClause || lastMapped < lastSkeleton[i]);
  }

private:

  static constexpr int BIGINT = 1000000;

  struct ClauseHash {
    size_t operator()(const std::vector<int> &clause) const {
      // This hash function is based on the hash function used by DRAT-trim.
      unsigned int sum = 0, prod = 1, xors = 0;
      for (auto literal: clause) {
        prod *= literal;
        sum += literal;
        xors ^= literal;
      }
      return (1023 * sum + prod ^ (31 * xors)) % BIGINT;
    }
  };

  // map a -> b
  void add_map (int a, int b) { 
    if (a >= stepIdxMap.size()) {
      stepIdxMap.resize (a+1, 0);
    }
    stepIdxMap[a] = b;
    // cout << a << " -> " << b << endl;
  }


  std::vector<Step*> steps;
  int nOriginal;

  std::vector<int>   stepIdxMap;
  std::vector<int>   lastSkeleton;
  int nFormulaClauses;
  int nSkeletonClauses;
  int nChunks;
  int lastMapped;
  bool emptyClause;

  
  std::unordered_set<std::vector<int>, ClauseHash> restored_clauses_;


};

int main(int argc, char* argv[]) {
  CLI::App app{
    "drat-parser: Parse DRAT files."
  };

  string proof_path = "";
  app.add_option("drat", proof_path,
                 "Path to a file of a DRAT proof."
  )->required();

  string map_path = "";
  app.add_option("drat", map_path,
                 "Path to a file of a DRAT proof."
  )->required()->check(CLI::ExistingFile);

  int nFormulaClauses;
  app.add_option("nFormula", nFormulaClauses,
                 "Number of clauses in original formula."
  )->required();

    int nSkeletonClauses;
  app.add_option("nSkeleton", nSkeletonClauses,
                 "Number of clauses in skeleton formula."
  )->required();

  int nChunks;
  app.add_option("nChunks", nChunks,
                 "Number of skeleton chunks."
  )->required();

  bool is_non_binary = false;
  app.add_flag("--no-binary", is_non_binary,
               "Parse proof in plain-text DRAT format.");

  CLI11_PARSE(app, argc, argv);

  LratMap           lrat(nFormulaClauses,nSkeletonClauses,nChunks);
  lrat.init_map (map_path);

  for (int i = 0; i < nChunks; i++) { // parse each chunk proof
    PlainTextDratParser drat_parser;
    drat_parser.AddObserver(&lrat);
    string temp = proof_path + std::to_string(i) + ".lrat";
    drat_parser.Parse(temp);
    if (lrat.early_conflict(i)) {
      break;
    }
  }
  
  // lrat.Post();

  return 0;
}

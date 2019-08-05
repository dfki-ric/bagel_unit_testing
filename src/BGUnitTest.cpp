#include "BGUnitTest.h"


#include <configmaps/ConfigData.h>
#include <mars/utils/misc.h>
#include <c_bagel/bagel.h>
#include <lib_manager/LibManager.hpp>

#include <cassert>
#include <cstdlib>
#include <float.h>
#include <cmath>

using namespace  configmaps;
using std::string;
using std::vector;
using std::list;

namespace bagel_unit_testing {

  struct sOrder {
    unsigned long id, i;
  };

  struct sBehavior {
    string name;
    string filename;
    string unitFilename;
  };

  int BGUnitTest::run() {
    list<sBehavior> behaviorList;

    lib_manager::LibManager *libManager = new lib_manager::LibManager();

    ConfigMap map;
    ConfigVector::iterator mapIt;
    map = ConfigMap::fromYamlFile("bagel_unit_testing.yml");

    string behaviorDef, behaviorFile, behaviorTestFile;
    bool valid;

    for(mapIt=(map["Behaviors"]).begin();
        mapIt!=(map["Behaviors"]).end(); ++mapIt) {
      behaviorDef = (*mapIt)["name"][0].getString();
      behaviorFile = behaviorDef+".yml";
      behaviorTestFile = behaviorDef+"_unit_test.yml";

      //if((valid = mars::utils::pathExists(behaviorFile))) {
        valid = mars::utils::pathExists(behaviorTestFile);
      //}

      if(valid) {
        behaviorList.push_back((sBehavior){behaviorDef, behaviorFile, behaviorTestFile});
      } else {
        fprintf(stderr, "ERROR files for given behavior \"%s\" not found: \n\t- %s\n\t- %s\n", behaviorDef.c_str(), behaviorFile.c_str(), behaviorTestFile.c_str());
      }  
    }

    bg_initialize();

    try {
      map = configmaps::ConfigMap::fromYamlFile("extern_bg_nodes.yml");
    }
    catch (...) {}

    configmaps::ConfigVector::iterator it;
    fprintf(stderr, "load extern nodes:\n");
    if (map.find("ExternBGNodes") != map.end()) {
        for (it = map["ExternBGNodes"].begin(); it != map["ExternBGNodes"].end();
                ++it) {
            std::string s = string(*it);

            s = libManager->findLibrary(s);
            load_extern_nodes(s.c_str());
            fprintf(stderr, " - %s\n", s.c_str());
        }
    }


    int failCount = 0;
    for(list<sBehavior>::iterator it=behaviorList.begin();
        it!=behaviorList.end(); ++it) {
      fprintf(stderr, "testing: %s ...\n", it->name.c_str());

      std::vector<bg_node_id_t> inputNodes;
      std::vector<bg_node_id_t> outputNodes;

      bg_graph_t *g;
      bg_error err;
      size_t inputCnt, outputCnt;

      bg_graph_alloc(&g, "graph");
      fprintf(stderr, "bg_graph_from_yaml: %s\n", it->filename.c_str());

      std::string filename = it->filename;
      std::string prefix = mars::utils::getPathOfFile(filename);
      mars::utils::removeFilenamePrefix(&filename);
      fprintf(stderr, "BehaviorGraph::loadGraph: %s / %s\n",
              prefix.c_str(), filename.c_str());
      if (!prefix.empty()) {
          bg_graph_set_load_path(g, prefix.c_str());
      }
      bg_graph_from_yaml_file(filename.c_str(), g);
      //bg_graph_from_yaml_file(it->filename.c_str(), g);
  
      bg_graph_get_input_nodes(g, 0, &inputCnt);
      bg_graph_get_output_nodes(g, 0, &outputCnt);
        
      printf("#inputs: %lu\n", inputCnt);
      printf("#outputs: %lu\n", outputCnt);

      bg_node_id_t inputIds[inputCnt];
      double inputs[inputCnt];
      double unitOutputs[outputCnt];
      double graphOutputs[outputCnt];
      bg_node_id_t outputIds[outputCnt];
      bg_graph_get_input_nodes(g, inputIds, &inputCnt);
      bg_graph_get_output_nodes(g, outputIds, &outputCnt);

      size_t edge_cnt;
      err =  bg_graph_get_edge_cnt(g, false, &edge_cnt);
      if(err != bg_SUCCESS) {
        fprintf(stderr, "c_bagel error: %d\n", err);
        assert(false);
      }

      list<sOrder> orderList;
      list<sOrder>::iterator oIt;
      std::vector<bg_node_id_t> tInputNodes;

      for(size_t i=0; i<inputCnt; ++i) {
        bg_edge_id_t edgeId = i+edge_cnt+1;
        fprintf(stderr, "add input id: %lu\n", inputIds[i]);
        err = bg_graph_create_edge(g, 0, 0, inputIds[i], 0, 1., edgeId);
        tInputNodes.push_back(edgeId);
        if(err != bg_SUCCESS) {
          fprintf(stderr, "c_bagel error: %d\n", err);
          assert(false);
        }

        for(oIt=orderList.begin(); oIt!=orderList.end(); ++oIt) {
          if(oIt->id > inputIds[i]) break;
        }
        orderList.insert(oIt, (sOrder){inputIds[i], i});
      }
      for(oIt=orderList.begin(); oIt!=orderList.end(); ++oIt) {
        inputNodes.push_back(tInputNodes[oIt->i]);
      }

      std::vector<bg_node_id_t>::iterator oNIt;
      for(unsigned int i=0; i<outputCnt; ++i) {
        for(oNIt=outputNodes.begin(); oNIt!=outputNodes.end(); ++oNIt) {
          if(*oNIt > outputIds[i]) break;
        }
        outputNodes.insert(oNIt, outputIds[i]);
      }
      
      map = ConfigMap::fromYamlFile(it->unitFilename);
      bool fail = false;
      bool error = false;
      double epsilon;
      FILE *unitLog = fopen("unitLog.log", "w");

      for(mapIt=(map["unit_tests"]).begin();
          mapIt!=(map["unit_tests"]).end(); ++mapIt) {
        if((*mapIt)["inputs"].size() != inputCnt) {
          fprintf(stderr, "ERROR: wrong number of inputs %lu\n",
                  (*mapIt)["inputs"].size());
          error = true;
        }
        if((*mapIt)["outputs"].size() != outputCnt) {
          fprintf(stderr, "ERROR: wrong number of outputs: %lu\n",
                  (*mapIt)["outputs"].size());
          error = true;
        }
        if(error) break;

        fprintf(stderr, "test inputs:");
        for(size_t i=0; i<inputCnt; ++i) {
          inputs[i] = (*mapIt)["inputs"][i];
          fprintf(stderr, " %g", inputs[i]);
          fprintf(unitLog, " %g", inputs[i]);
          err = bg_edge_set_value(g, inputNodes[i], inputs[i]);
          if(err != bg_SUCCESS) {
            fprintf(stderr, "c_bagel error: %d\n", err);
            assert(false);
          }
        }
        fprintf(stderr, "\n");

        err = bg_graph_evaluate(g);
        if(err != bg_SUCCESS) {
          fprintf(stderr, "c_bagel error: %d\n", err);
          assert(false);
        }
        
        for(size_t i=0; i<outputCnt; ++i) {
          unitOutputs[i] = (*mapIt)["outputs"][i];
          err = bg_node_get_output(g, outputNodes[i], 0, graphOutputs+i);
          if(err != bg_SUCCESS) {
            fprintf(stderr, "c_bagel error: %d\n", err);
            assert(false);
          }

          if(mapIt->hasKey("epsilon")) {
            epsilon = (*mapIt)["epsilon"][i];
          }
          else epsilon = 0.0001;

          if(fabs(unitOutputs[i]-graphOutputs[i]) > epsilon) {
            fprintf(stderr, "expected output (%lu): %g \t graph output (%lu): %g\n", i,
                    unitOutputs[i], outputNodes[i], graphOutputs[i]);
            fail = true;
          }
          fprintf(unitLog, " %g %g", unitOutputs[i], graphOutputs[i]);
        }
        fprintf(unitLog, "\n");

        if(fail) {
          //fprintf(stderr, "... fail\n");
          //break;
        }

      }
      fclose(unitLog);
      if(fail) fprintf(stderr, "... fail\n");
      if(!fail) fprintf(stderr, "... fine\n");
    }

    return failCount;
  }

} /* end of namespace bagel_unit_testing */

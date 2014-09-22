#include <iostream>
#include <map>

using namespace std;

int main(){
  //estas variables a la libreria
  typedef std::map<int, string> localmap;
  typedef std::map<string, localmap> globalmap;

  globalmap global_topicMap;

  std::string str("mac");
  std::string topicname("a/b");
  int topicid = 222;
  int topic_id_count = 0; //uint16_t
  std::map<string, localmap>::const_iterator search;

  for(int i=0;i<4;i++) {

    search = global_topicMap.find(str);

    if(search != global_topicMap.end()) {

      std::cout << "mac encontrada\n";

      localmap aux2 = search->second;
      std::map<int, string>::const_iterator s = aux2.find(topic_id_count);

      if(s != aux2.end()) {
	std::cout << "rejected invalid topic id\n";
      } else {
	aux2[topic_id_count] = topicname;
	global_topicMap[str] = aux2;
	topic_id_count++;
      }

    }
    else {
      std::cout << "mac no encontrada\n";
      localmap local_topicMap;
      local_topicMap.insert(std::make_pair(topic_id_count, topicname));
      global_topicMap.insert(std::make_pair(str, local_topicMap));
      topic_id_count++;
    }
  }

  //print
  std::cout << "global topic map\n";
  for(std::map<string,localmap>::const_iterator it = global_topicMap.begin(); it!=global_topicMap.end(); ++it) {
    std::cout << it->first << "->";
    localmap aux = it->second;
    for(std::map<int,string>::const_iterator it2 = aux.begin(); it2!=aux.end(); ++it2) {
      std::cout << it2->first << ":" << it2->second << std::endl;
    }
  }

  return 1;//topic_id_count-1
}

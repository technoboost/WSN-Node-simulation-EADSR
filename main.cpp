#include <iostream> 
#include <thread> 
#include <chrono>
#include <unistd.h>
#include <mqueue.h>
#include <string>
#include <string.h>
#include <fstream>
#include <fcntl.h>         
#include <sys/stat.h> 
#include <list>
#include <vector>
#include<bits/stdc++.h> 
#include <utility>
using namespace std; 
typedef pair<int,int> iPair; 
//#define DEBUG_LOG_ENABLE

#define NODE_COUNT 6
#define DEBUG_ERROR_LOG_ENABLE
#define SERVER_QUEUE_NAME   "/nodeclient-1"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10
#define PACKET_SIZE 19
#define MAX_NEIGHBORS 20
#define BASE_ID 0x01
#define NUM_NODES NODE_COUNT
# define INF 0x3f3f3f3f 
static struct timespec RcvCmdWaitTime;
const char nodeid[NUM_NODES]={0x01,0x02,0x03,0x04,0x05,0x06};
const int nodeegy[NUM_NODES]={0x24,0x34,0x64,0x21,0x45,0x52};
typedef struct{
	char node[10];
	char node_energy[10];
	char neigh[100];
}route_t;
route_t route[NODE_COUNT];
int sendMessageToNode(int nodeNumber, int message,mqd_t qd_server);
list<int> getNeighbours(int nodeNumber);
typedef int vertex_t;
typedef double weight_t;

class BaseStation 
{
private:
	int V; 
	list<int> neighbours;
	string messageQueueRcv;
	char packet[PACKET_SIZE];
	const weight_t max_weight = std::numeric_limits<double>::infinity();
 
	struct neighbor {
    vertex_t target;
    weight_t weight;
    neighbor(vertex_t arg_target, weight_t arg_weight)
        : target(arg_target), weight(arg_weight) { }
	};
	typedef std::vector<std::vector<neighbor> > adjacency_list_t;
public:
	mqd_t messageQueueHandles[NODE_COUNT];
	int init();
	int last_node_server(char *packet);
	
	void startListening();
	void sendBroadcast();
	void DijkstraComputePaths(vertex_t source,
                          const adjacency_list_t &adjacency_list,
                          std::vector<weight_t> &min_distance,
                          std::vector<vertex_t> &previous);
	list<vertex_t> DijkstraGetShortestPathTo(vertex_t vertex, const std::vector<vertex_t> &previous);
	BaseStation(int Ver);
};
class Node
{
private:
	list<int> neighbours;
	int id;
	int energy;
	string messageQueueRcv;
	char message[60];
	char packet[PACKET_SIZE];
public:
		void setId(int id, int energy){
			this->id= id;
			this->energy=energy;
			messageQueueRcv = "/nodeclient-";
			messageQueueRcv += to_string(id);
			
		}
		int init();
		void startListening();
		bool last_node(char *packet);
		bool node_in_packet(char *packet, int *locat);
		void packet_arrange_sync(char *packet);
		char* find_neighbor(char *packet);
		char* ack_neighbor(char *packet);
};
int last_node_server(char *packet)
{
    int i;
    for(i=6;i<16;i++)
    {
        if(packet[i]==0x00)
            break;
    }
    if((packet[i+1]==0x56))
    {
        return packet[i-1];
    }
    return 0;
}

int idtoindex(int l, int r, int x) //binary search algorithm
{ 
   if (r >= l) 
   { 
        int mid = l + (r - l)/2; 
  
        if (nodeid[mid] == x)   
            return mid; 
  
        if (nodeid[mid] > x)  
            return idtoindex(l, mid-1, x); 
  
        return idtoindex(mid+1, r, x); 
   } 
  
   return -1; 
} 
bool notinandappend(char value, char* neigh)
{
	int i;
	for (i=0;i<100;i++)
	{
		if(value==*(neigh))
		{
			return false;
		}
		if (*neigh==0x00)
		{
			*neigh=value;
			break;
		}
		neigh++;
	}
	return true;
}
void sortascending(char* neigh)
{
	int i,j,n;
	char key;
	for(i=0;i<100;i++)
	{
		if (*(neigh++)==0x00)
			break;
	}
	n=i;
	neigh-=(n+1);
	for (i = 1; i < n; i++) //insertion sort
   	{ 
       key = *(neigh+i); 
       j = i-1; 
       while (j >= 0 && *(neigh+j) > key) 
       { 
           *(neigh+j+1) = *(neigh+j); 
           j = j-1; 
       } 
       *(neigh+j+1) = key; 
   } 
	
	
}
bool waitfornack(char* ptr)
{
	int flag=0,i,j;
	char packet[19];
	for(i=0;i<19;i++)
	   	packet[i]=*ptr++;
	ptr-=19;
	
	if((packet[PACKET_SIZE-1]=='P')&&(packet[PACKET_SIZE-2]=='O')&&(packet[PACKET_SIZE-3]=='E')) //whether we have completed reception
        {
            if((packet[1]=='N')&&(packet[2]=='E')&&(packet[3]=='A')&&(packet[4]=='C')&&(packet[5]=='K'))
            {
                if(packet[0]=='#')
                {
                    if(last_node_server(packet)==BASE_ID)
                	{
                		if((notinandappend(packet[6],route[idtoindex(0,NUM_NODES-1,packet[8])].neigh))==true)
                		{
                			sortascending(route[idtoindex(0,NUM_NODES-1,packet[8])].neigh);
                			return true;
                		}
                	}	
                }
            }
        }
    return false;
}
bool Node::last_node(char *packet)
{
    int i;
    for(i=6;i<16;i++)
    {
        if(packet[i]==0x00)
            break;
    }
    if((packet[i+1]==0x00)&&(i%2==0))
    {
        return true;
    }
    return false;
}

bool Node::node_in_packet(char *packet, int *locat)
{
    int i;
    for(i=6;i<16;i+=2)
    {
        if(packet[i]==id)
        {
            *locat=i;
            return true;
        }
    }
    return false;
}
void Node::packet_arrange_sync(char *packet)
{
    int i,j;
    char tmp[10];
    for(i=6;i<16;i++)
    {
        if(packet[i]==0x00)
            break;
    }
    packet[i]=id;
    packet[++i]=energy;
    for(j=6;j<=i;j+=2)
    {
        tmp[j-6]=packet[i-j+5];//node
        tmp[j-5]=0x00;//energy
    }
    for(j=6;j<=i;j++)
        packet[j]=tmp[j-6];
    packet[7]=energy;
    for(j=i+1;j<16;j++)
        packet[j]=0x56;
}
char* Node::find_neighbor(char *packet)
{
    int i,loc=0;
    if(node_in_packet(packet,&loc))
    {
        packet[loc+1]=energy;
        return packet;
    }
	if(last_node(packet))
    {
        *(packet+1)='N';
        *(packet+2)='E';
        *(packet+3)='A';
        *(packet+4)='C';
        *(packet+5)='K';
        packet_arrange_sync(packet);
        return packet;
    }
    return NULL;
//location1:;
}

char* Node::ack_neighbor(char *packet)
{
    int i,loc=0;
    if(node_in_packet(packet,&loc))
    {
		if(packet[loc+1]==0x00)
		{
			packet[loc+1]=energy;
			return packet;
		}
    }
    return NULL;
}

list<int> getNeighbours(int nodeNumber)
{
	list<int> neighbours;
	if(nodeNumber == 1)
	{
		neighbours.push_back(2);
		neighbours.push_back(5);
	}
	else if(nodeNumber == 2)
	{
		neighbours.push_back(1);
		neighbours.push_back(3);
	}
	else if(nodeNumber == 3)
	{
		neighbours.push_back(2);
		neighbours.push_back(4);
		neighbours.push_back(5);
		neighbours.push_back(6);
	}
	else if(nodeNumber == 4)
	{
		neighbours.push_back(3);
		neighbours.push_back(5);
	}
	else if(nodeNumber == 5)
	{
		neighbours.push_back(1);
		neighbours.push_back(3);
		neighbours.push_back(4);
		neighbours.push_back(6);
	}
	else if(nodeNumber == 6)
	{
		neighbours.push_back(5);
		neighbours.push_back(3);
	}
	else
	{
	}
	return neighbours;
}
BaseStation::BaseStation(int V) 
{ 
    this->V = V; 
    
} 

void BaseStation::DijkstraComputePaths(vertex_t source,
                          const adjacency_list_t &adjacency_list,
                          std::vector<weight_t> &min_distance,
                          std::vector<vertex_t> &previous)
{
    int n = adjacency_list.size();
    min_distance.clear();
    min_distance.resize(n, max_weight);
    min_distance[source] = 0;
    previous.clear();
    previous.resize(n, -1);
    std::set<std::pair<weight_t, vertex_t> > vertex_queue;
    vertex_queue.insert(std::make_pair(min_distance[source], source));
 
    while (!vertex_queue.empty()) 
    {
        weight_t dist = vertex_queue.begin()->first;
        vertex_t u = vertex_queue.begin()->second;
        vertex_queue.erase(vertex_queue.begin());
 
        // Visit each edge exiting u
		const std::vector<neighbor> &neighbors = adjacency_list[u];
        for (std::vector<neighbor>::const_iterator neighbor_iter = neighbors.begin();
             neighbor_iter != neighbors.end();
             neighbor_iter++)
        {
            vertex_t v = neighbor_iter->target;
            weight_t weight = neighbor_iter->weight;
            weight_t distance_through_u = dist + weight;
	    if (distance_through_u < min_distance[v]) {
	        vertex_queue.erase(std::make_pair(min_distance[v], v));
 
	        min_distance[v] = distance_through_u;
	        previous[v] = u;
	        vertex_queue.insert(std::make_pair(min_distance[v], v));
 
	    }
 
        }
    }
}
 
 
std::list<vertex_t> BaseStation::DijkstraGetShortestPathTo(
    vertex_t vertex, const std::vector<vertex_t> &previous)
{
    std::list<vertex_t> path;
    for ( ; vertex != -1; vertex = previous[vertex])
        path.push_front(vertex);
    return path;
}
int Node::init()
{
}
int BaseStation::init()
{
	
}

void Node::startListening(){
	char client_queue_name [64];
	char *packet;
    mqd_t qd_client,qd_forward;   
    sprintf (client_queue_name, "/nodeclient-%d", id);

    struct mq_attr attr;
	RcvCmdWaitTime.tv_sec = 0;
    RcvCmdWaitTime.tv_nsec = 250000;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    string logfile = "node";
	logfile += to_string(id);
	logfile +=".txt";
    ofstream myfile (logfile.c_str());
    myfile.close();

    

	//cout <<"opened client queue  :"<<id<<endl;
    char in_buffer [MSG_BUFFER_SIZE];

    char temp_buf [10];

    while (1) {
		if ((qd_client = mq_open (client_queue_name, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror ("Client: mq_open (client)");
        exit (1);
		}
        if (mq_receive (qd_client, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
            perror ("Client: mq_receive");
            exit (1);
        }
        myfile.open(logfile.c_str(),ios::app);
		//cout <<"Received : " <<id<<":";
		for(int i=0;i<19;i++)
		{
			//printf(" %x", in_buffer[i]);
		 	myfile << in_buffer[i];
		}
		myfile << "\n";
		myfile.close();
					
		//cout<<endl;
		if(id!=1)
		{
			if((in_buffer[1]=='N')&&(in_buffer[2]=='E')&&(in_buffer[3]=='I')&&(in_buffer[4]=='G')&&(in_buffer[5]=='H'))
			{
				packet=find_neighbor(in_buffer);
				if(packet!=NULL)
				{
					neighbours = getNeighbours(id);
					list <int> :: iterator it;
					
					for(it = neighbours.begin(); it != neighbours.end(); ++it)
					{
						messageQueueRcv = "/nodeclient-";
						messageQueueRcv+=to_string(*it);
						if ((qd_forward = mq_open (messageQueueRcv.c_str(), O_WRONLY)) == -1) {
						perror ("Forward: mq_open (qdforward)");
						continue;
						}
						if (mq_timedsend (qd_forward, packet, PACKET_SIZE, 0, &RcvCmdWaitTime) == -1) {
							perror ("Forward: Not able to send message to client");
							mq_close(qd_forward);
							char temp='&';
							while(temp=='&')
							{
								temp='*';
								if ((qd_forward = mq_open (messageQueueRcv.c_str(), O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
								perror ("Forward: mq_open (qdforward)");
								continue;
								}
								if (mq_timedreceive (qd_forward, in_buffer, MSG_BUFFER_SIZE, 0,&RcvCmdWaitTime) == -1) {
									perror ("forward: Not able to receive message from client");
								}
								else{
									temp='&';
								}
								mq_close(qd_forward);
							}
						}
						//cout << "opened message queue first: " <<messageQueueRcv.c_str()<<endl;

						/*cout <<"first";
						for(int i=0;i<19;i++)
							printf(" %x", packet[i]);*/
						
						//cout<<endl;
						mq_close(qd_forward);
					
					}
				}
			}
			else if((in_buffer[1]=='N')&&(in_buffer[2]=='E')&&(in_buffer[3]=='A')&&(in_buffer[4]=='C')&&(in_buffer[5]=='K'))
			{
				packet=ack_neighbor(in_buffer);
				if(packet!=NULL)
				{
					neighbours = getNeighbours(id);
					list <int> :: iterator it;
					
					for(it = neighbours.begin(); it != neighbours.end(); ++it)
					{
						messageQueueRcv = "/nodeclient-";
						messageQueueRcv+=to_string(*it);
						if ((qd_forward = mq_open (messageQueueRcv.c_str(), O_WRONLY)) == -1) {
						perror ("Forward: mq_open (qdforward)");
						continue;
						}
						//cout << "opened message queue forward: " <<messageQueueRcv.c_str()<<endl;
						if (mq_timedsend (qd_forward, packet, PACKET_SIZE, 0,&RcvCmdWaitTime) == -1) {
							perror ("Forward: Not able to send message to client");
							mq_close(qd_forward);
							char temp='&';
							while(temp=='&')
							{
								temp='*';
								if ((qd_forward = mq_open (messageQueueRcv.c_str(), O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
								perror ("Forward: mq_open (qdforward)");
								continue;
								}
								if (mq_timedreceive (qd_forward, in_buffer, MSG_BUFFER_SIZE, 0,&RcvCmdWaitTime) == -1) {
									perror ("forward: Not able to receive message from client");
								}
								else{
									temp='&';
								}
								mq_close(qd_forward);
							}
						}
						mq_close(qd_forward);
					}
				}
			}
			else{}
		}
		else
		{
			if(waitfornack(in_buffer))
			{
				//printf("Here\n");
			}
		}
								std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (mq_close (qd_client) == -1) {
        perror ("Client: mq_close");
        exit (1);
		}
    }

    if (mq_unlink (client_queue_name) == -1) {
        perror ("Client: mq_unlink");
        exit (1);
    }
    printf ("Client: bye\n");
}
void BaseStation::sendBroadcast()
{
 	mqd_t qd_client[MAX_NEIGHBORS],qd_cliafloop;   // queue descriptors
	vector <int> iternodelist;
	vector <int> calcnodelist;
	vector <int> difflist;
	vector<int>::iterator ip;
	std::vector<weight_t> min_distance;
    std::vector<vertex_t> previous;
	adjacency_list_t adjacency_list(V+1);
	//int whilecount=0;
    printf ("Server: Hello, World!\n");

    struct mq_attr attr;
	RcvCmdWaitTime.tv_sec = 0;
    RcvCmdWaitTime.tv_nsec = 250000;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    neighbours = getNeighbours(1);
	list <int> :: iterator it;
	int looper=0;
	for(it = neighbours.begin(); it != neighbours.end(); ++it)
	{
		messageQueueRcv = "/nodeclient-";
		messageQueueRcv+=to_string(*it);
		if ((qd_client[looper] = mq_open (messageQueueRcv.c_str(), O_WRONLY)) == 1) {
				perror ("Server: Not able to open client queue");
		}
		looper++;
	}
	int fstnodeneighno=looper;
    char in_buffer [MSG_BUFFER_SIZE];
	packet[0]='#';
	packet[1]='N';
	packet[2]='E';
	packet[3]='I';
	packet[4]='G';
	packet[5]='H';
	packet[6]=0x01;
	packet[7]=0x24;
	packet[8]=0x00;
	packet[9]=0x00;
	packet[10]=0x00;
	packet[11]=0x00;
	packet[12]=0x00;
	packet[13]=0x00;
	packet[14]=0x00;
	packet[15]=0x00;
	packet[16]='E';
	packet[17]='O';
	packet[18]='P';
	for(int loop=0;loop<fstnodeneighno;loop++)
	if (mq_send (qd_client[loop], packet, PACKET_SIZE, 0) == -1) {
		perror ("Server: Not able to send message to client");
		continue;
	}
	
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	iternodelist.push_back(1);
	set_difference(iternodelist.begin(), iternodelist.end(), calcnodelist.begin(), calcnodelist.end(),inserter(difflist, difflist.begin()));
    while (1) {
		//cout<<"in while :"<<whilecount++<<endl;
		while(!difflist.empty())
		{
			for (auto ite : difflist)
			{	
				for(int j=0;route[ite-1].neigh[j]!=0;j++)
				{
					adjacency_list[ite].push_back(neighbor(route[ite-1].neigh[j],1));
					if (find(iternodelist.begin(), iternodelist.end(), route[ite-1].neigh[j]) == iternodelist.end()) {
						iternodelist.push_back(route[ite-1].neigh[j]);
					}
				}
				for (int i=1;i<NUM_NODES;i++)
				{
					for(int j=0;route[i].neigh[j]!=0;j++)
					{
						if((notinandappend(i+1,route[idtoindex(0,NUM_NODES-1,route[i].neigh[j])].neigh))==true)
							{
								sortascending(route[idtoindex(0,NUM_NODES-1,route[i].neigh[j])].neigh);
							}
					}
				}
				//cout<<"Element in diff list :"<<ite<<endl;
				sort(iternodelist.begin(), iternodelist.end());
				ip = unique(iternodelist.begin(), iternodelist.end()); 
				iternodelist.resize(distance(iternodelist.begin(), ip));
				/*for(auto irt=iternodelist.begin();irt!=iternodelist.end();irt++)
				{
					cout << "Iterbnid :" <<*irt<<endl;
				}*/
				DijkstraComputePaths(1, adjacency_list, min_distance, previous);
				list<vertex_t> path = DijkstraGetShortestPathTo(ite, previous);
				cout << "Path : ";
				looper=0;
				for (std::list<int>::const_iterator iterator = path.begin(), end = path.end(); iterator != end; ++iterator) {
					route[ite-1].node[looper]=*iterator;
					route[ite-1].node_energy[looper]=0;/*nodeegy[*iterator];*/
					std::cout << *iterator<<" ";
					looper++;
				}

				//copy(path.begin(), path.end(), std::ostream_iterator<vertex_t>(std::cout, " "));
				cout << std::endl;
				packet[0]='#';
				packet[1]='N';
				packet[2]='E';
				packet[3]='I';
				packet[4]='G';
				packet[5]='H';
				packet[6]=route[ite-1].node[0];
				packet[7]=0x24;//route[ite-1].node_energy[0];
				packet[8]=route[ite-1].node[1];
				packet[9]=route[ite-1].node_energy[1];
				packet[10]=route[ite-1].node[2];
				packet[11]=route[ite-1].node_energy[2];
				packet[12]=route[ite-1].node[3];
				packet[13]=route[ite-1].node_energy[3];
				packet[14]=route[ite-1].node[4];
				packet[15]=route[ite-1].node_energy[4];
				packet[16]='E';
				packet[17]='O';
				packet[18]='P';
				cout<<"Hello George"<<endl;
				for(int j=0;j<19;j++)
				{
					printf("%x  ",packet[j] );
				}
				cout<<endl;
				for(int loop=0;loop<fstnodeneighno;loop++)
				if (mq_timedsend (qd_client[loop], packet, PACKET_SIZE, 0,&RcvCmdWaitTime) == -1) {
					perror ("Server: Not able to send message to client");
					continue;
				}
				
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
				for(int i=0;i<NUM_NODES;i++)
				{
					for(int j=0;j<5;j++)
						{
							printf("%x  ",route[i].neigh[j] );
						}
						cout<<endl;
				}
				for(int j=0;route[ite-1].neigh[j]!=0;j++)
				{
					adjacency_list[ite].push_back(neighbor(route[ite-1].neigh[j],1));
					if (find(iternodelist.begin(), iternodelist.end(), route[ite-1].neigh[j]) == iternodelist.end()) {
						iternodelist.push_back(route[ite-1].neigh[j]);
					}
				}
				//cout<<"Element in diff list :"<<ite<<endl;
				sort(iternodelist.begin(), iternodelist.end());
				ip = unique(iternodelist.begin(), iternodelist.end()); 
				iternodelist.resize(distance(iternodelist.begin(), ip));
				/*for(auto irt=iternodelist.begin();irt!=iternodelist.end();irt++)
				{
					cout << "Iterbnid :" <<*irt<<endl;
				}*/
				calcnodelist.push_back(ite);
				sort(calcnodelist.begin(), calcnodelist.end());
				ip = unique(calcnodelist.begin(), calcnodelist.end()); 
				calcnodelist.resize(distance(calcnodelist.begin(), ip));
				for(auto irt=calcnodelist.begin();irt!=calcnodelist.end();irt++)
				{
					cout << "Calcnodelist :" <<*irt<<endl;
				}
			}
			while(!difflist.empty())
			{
				difflist.pop_back();
			}
			set_difference(iternodelist.begin(), iternodelist.end(), calcnodelist.begin(), calcnodelist.end(),inserter(difflist, difflist.begin()));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		/*char temp='&';
		while(difflist.empty()&& (temp=='&'))
		{
			temp='*';
			for(int i=1;i<=NUM_NODES;i++)
			{
				messageQueueRcv = "/nodeclient-";
				messageQueueRcv+=to_string(i);
				if ((qd_cliafloop = mq_open (messageQueueRcv.c_str(), O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
					perror ("after while: mq_open (qdclient)");
				}
				if (mq_timedreceive (qd_cliafloop, in_buffer, MSG_BUFFER_SIZE, 0,&RcvCmdWaitTime) == -1) {
					perror ("after while: Not able to receive message from client");
					cout<<messageQueueRcv;
					cout<<" "<<i<<endl;
				}
				else{
					temp='&';
				}
				mq_close(qd_cliafloop);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
		*/
	}
}


int main() 
{

	Node nodes[NODE_COUNT];
	int Ver=NUM_NODES;
	BaseStation baseStation(Ver);
	thread nodeListening[NODE_COUNT];
	for(int i=0;i<NUM_NODES;i++)
    {
	   	for(int j=0;j<10;j++)
		{
			route[i].node[j]=0x00;
			route[i].node_energy[j]=0x00;
		}
		for(int j=0;j<100;j++)
		{
			route[i].neigh[j]=0x00;
		}
    }
	for(int i=0; i< NODE_COUNT; i++){
		nodes[i].setId(i+1,nodeegy[i]);
	}

	baseStation.init();
	for(int i=0; i< NODE_COUNT; i++){
		nodes[i].init();
	}
	
	for(int i=0; i< NODE_COUNT; i++){
		nodeListening[i]= std::thread (&Node::startListening,&nodes[i]);
	
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	baseStation.sendBroadcast();
	for(int i=0; i< NODE_COUNT; i++){
		nodeListening[i].join();
	}

	return 0; 
} 
	

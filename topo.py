#!/usr/bin/python
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel
from mininet.cli import CLI

LOSS=0
GRAPH={'s1':['h2'],'s2':['h4'],'s3':['h1','h3']}
class SingleSwitchTopo( Topo ):
    "Single switch connected to n hosts."
    def build(self):
        switches = [0]+[self.addSwitch( 's'+str(i) ) for i in range(1,4)]
        self.addLink(switches[1],switches[2],bw=10,loss=LOSS)
        self.addLink(switches[1],switches[3],bw=10,loss=LOSS)
        # self.addLink(switches[2],switches[3],bw=10,loss=LOSS)

        for switch,hosts in GRAPH.items():
            for hostname in hosts:
                host=self.addHost(hostname)
                self.addLink(host,switches[int(switch[1])])
        # for h in range(n):
        #     # Each host gets 50%/n of system CPU
        #     host = self.addHost( 'h%s' % (h + 1))
            # # 10 Mbps, 5ms delay, 2% loss, 1000 packet queue
            # self.addLink( host, switch, bw=10, delay='0ms', loss=0,
            #               max_queue_size=1000, use_htb=True )

def perfTest():
    "Create network and run simple performance test"
    topo = SingleSwitchTopo( )
    net = Mininet( topo=topo,link=TCLink)
    net.start()
    print( "Dumping host connections" )
    dumpNodeConnections( net.hosts )
    CLI(net)
    # print( "Testing network connectivity" )
    # net.pingAll()
    # print( "Testing bandwidth between h1 and others" )
    # h1 = net.get( 'h1' )
    # for i in range(2,7):
    #     hi =net.get('h'+str(i))
    #     net.iperf( (h1, hi) )
    net.stop()


if __name__ == '__main__':
    setLogLevel( 'info' )
    perfTest()
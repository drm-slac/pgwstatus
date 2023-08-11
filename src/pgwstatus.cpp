/*
 * display EPICS p4p gateway diagnostic PVs
 */
#include <string>
#include <iostream>

#include <getopt.h>
#include <pvxs/client.h>

using namespace pvxs;
using namespace std;

class GatewayStatus
        {
        public:
                GatewayStatus();
                ~GatewayStatus();

                void setGatewayName( string gatewayName);

                bool showPVConnections();
                bool showIOCConnections();
                bool showPVRates( bool isDownstream, bool findTransmitRate);
                bool showHostRates( bool isDownstream, bool findTransmitRate);

                std::string convertFromIPAddr( std::string ipAddr);
        private:
                bool showRPCResults( string pvName);
                bool showPermissions( pvxs::Value result);
                bool getPVData( string pvName, string errContext, pvxs::Value &pvData);

                string _gatewayName;
                client::Context _context;
        };

unsigned int
arguments( int argc, char *const argv[])
        {
        int c;

	while (( c = getopt( argc, argv, "u:h")) != -1)
                {
		switch (c)
                        {
		case 'u':
			fprintf( stdout, "Option 'u': '%s'\n", optarg);
			break;

		case 'h':
                        fprintf( stdout, "Usage: %s gateway-name ...\n", argv[0]);
                        exit( 0);
                        break;

		default:
			fprintf( stderr, "Illegal argument \"%c\"\n", c);
			exit( 1);
                        }
                }

        return optind;
        }


int
main( int argc, char *const argv[])
        {
        unsigned int i;
        unsigned int nargs = argc;

        GatewayStatus status;

        for( i = arguments( argc, argv); i < nargs; i++)
                {
                status.setGatewayName( argv[i]);

                if( ! status.showIOCConnections())
                        continue;

                (void)status.showPVConnections();

                fprintf( stdout, "    Client and IOC Transfer Rates :\n");
                fprintf( stdout, "        Gateway's Server Side:\n");
                (void)status.showHostRates( true, true);
                (void)status.showHostRates( true, false);
                fprintf( stdout, "        Gateway's Client Side:\n");
                (void)status.showHostRates( false, false);
                (void)status.showHostRates( false, true);

                fprintf( stdout, "    PV Transfer Rates:\n");
                fprintf( stdout, "        Gateway's Server Side:\n");
                (void)status.showPVRates( true, true);
                (void)status.showPVRates( true, false);
                fprintf( stdout, "        Gateway's Client Side:\n");
                (void)status.showPVRates( false, false);
                (void)status.showPVRates( false, true);
                }

        return 0;
        }

GatewayStatus::
GatewayStatus() :
_context( client::Context::fromEnv())
        {
        }

GatewayStatus::
~GatewayStatus()
        {
        _context.close();
        }

void GatewayStatus::
setGatewayName( string gatewayName)
        {

        _gatewayName = gatewayName;
        fprintf( stdout, "Gateway %s:\n", _gatewayName.c_str());
        }

bool GatewayStatus::
getPVData( string pvName, string errContext, pvxs::Value &pvData)
        {

        try
                {
                Value result = _context.get( pvName.c_str()).exec()->wait( 5.0);
                pvData = result;
                }
            catch( client::Timeout &)
                {
                fprintf( stdout, "    Attempt to %s could not be completed.\n", errContext.c_str());
                return false;
                }

        if( ! pvData.valid())
                {
                fprintf( stdout, "    Failed to %s\n", errContext.c_str());
                return false;
                }

        return true;
        }

bool GatewayStatus::
showIOCConnections()
        {
        Value reply;
        unsigned int i;
        string clientPV = _gatewayName + ":clients";

        if( ! getPVData( clientPV, "find IOC connections", reply))
                return false;

        shared_array<const string> val = reply["value"].as<shared_array<const string>>();

        if( val.size() > 0)
                fprintf( stdout, "    Access to %lu IOCs:\n", val.size());
            else
                fprintf( stdout, "    No IOCs currently accessed\n");

        for( i = 0; i < val.size(); i++)
                {
                string host = convertFromIPAddr( val[i]);
                fprintf( stdout, "        %s\n", host.c_str());
                }
        return true;
        }

bool GatewayStatus::
showPVConnections()
        {
        Value reply;
        unsigned int i;
        string clientPV = _gatewayName + ":cache";

        if( ! getPVData( clientPV, "find PV data", reply))
                return false;

        shared_array<const string> val = reply["value"].as<shared_array<const string>>();

        if( val.size() > 0)
                fprintf( stdout, "    Access to %lu PVs:\n", val.size());
            else
                fprintf( stdout, "    No PVs currently accessed\n");
        for( i = 0; i < val.size(); i++)
                {
                string pvName = val[i];
                fprintf( stdout, "        %s", pvName.c_str());
                fprintf( stdout, "\n");
                showRPCResults( pvName);
                }

        return true;
        }

bool GatewayStatus::
showRPCResults( string pvName)
        {
        Value reply;
        string clientPV = _gatewayName + ":asTest";

        try
                {
                Value result = _context.rpc( clientPV.c_str()).arg( "pv", pvName).exec()->wait( 5.0);
                reply = result;
                }
            catch( client::Timeout &)
                {
                fprintf( stdout, "       Cannot access gateway '%s' for details on '%s'\n", _gatewayName.c_str(), pvName.c_str());
                return false;
                }
            catch( ...)
                {
                fprintf( stdout, "       Cannot retrieve details for %s'\n", pvName.c_str());
                return false;
                }

        if( ! reply.valid())
                {
                fprintf( stdout, "       No details available for '%s' from gateway '%s'\n", pvName.c_str(), _gatewayName.c_str());
                return false;
                }

        string pv = reply["pv"].as<string>();
        if( pv != pvName)
                fprintf( stdout, "       <<Software Error>> Requested '%s' details but found '%s'.  Attempting to continue.\n", pvName.c_str(), pv.c_str());

        fprintf( stdout, "            requested by: %s on %s\n", reply["account"].as<string>().c_str(), convertFromIPAddr( reply["peer"].as<string>()).c_str());
        fprintf( stdout, "            ASG applied: %s (ASL %d)\n", reply["asg"].as<string>().c_str(), reply["asl"].as<int32_t>());

        showPermissions( reply);
        return true;
        }

bool GatewayStatus::
showPermissions( pvxs::Value fields)
        {
        Value perms = fields["permission"];

        fprintf( stdout, "            PUT operations are %sallowed\n", perms["put"].as<bool>() ? "" : "NOT ");
        fprintf( stdout, "            RPC operations are %sallowed\n", perms["rpc"].as<bool>() ? "" : "NOT ");

        //for( Value fld : perms.ichildren())
                //{
                //std::cout << "*** FOUND " << perms.nameOf( fld) <<" : "<<fld<<"\n";
                //}
        return true;
        }

bool GatewayStatus::
showHostRates( bool isDownstream, bool findTransmitRate)
        {
        string errContext;
        string clientPV = _gatewayName;

        if( isDownstream)
                {
                clientPV += ":ds:byhost";
                errContext = "IOCs";
                }
            else
                {
                clientPV += ":us:byhost";
                errContext = "connected clients";
                }

        if( findTransmitRate)
                {
                clientPV += ":tx";
                errContext = "retrieve transmission rates to" + errContext;
                }
            else
                {
                clientPV += ":rx";
                errContext = "retrieve reception rates from" + errContext;
                }

        Value dataRates;
        
        if( ! getPVData( clientPV, errContext, dataRates))
                return false;

        Value rateDetails = dataRates["value"];

        shared_array<const string> user;
        unsigned long sizeUser = 0L;

        shared_array<const string> host;
        unsigned long sizeHost;

        shared_array<const double> rate;
        unsigned long sizeRate;

        if( isDownstream)
                {
                user = rateDetails["account"].as<shared_array<const string>>();
                sizeUser = user.size();
                }

        host = rateDetails["name"].as<shared_array<const string>>();
        sizeHost = host.size();

        rate = rateDetails["rate"].as<shared_array<const double>>();
        sizeRate = rate.size();

        if( sizeHost != sizeRate || ( isDownstream && ( sizeUser != sizeHost)))
                {
                fprintf( stdout, "            Software Error: data rate details are inconsistent. (%lu hosts, %lu speeds)\n", sizeHost, sizeRate);
                return false;
                }

        for( unsigned int i = 0; i < sizeHost; i++)
                {
                string hostName = convertFromIPAddr( host[i]);

                if( findTransmitRate)
                        {
                        if( isDownstream)
                                fprintf( stdout, "            Gateway transmits %.4g bytes/second  to  %s on %s\n", rate[i], user[i].c_str(), hostName.c_str());
                            else
                                fprintf( stdout, "            Gateway transmits %.4g bytes/second  to  %s\n", rate[i], hostName.c_str());
                        }
                    else
                        {
                        if( isDownstream)
                                fprintf( stdout, "            Gateway receives  %.4g bytes/second from %s on %s\n", rate[i], user[i].c_str(), hostName.c_str());
                            else
                                fprintf( stdout, "            Gateway receives  %.4g bytes/second from %s\n", rate[i], hostName.c_str());
                        }
                }

        return true;
        }

bool GatewayStatus::
showPVRates( bool isDownstream, bool findTransmitRate)
        {
        string errContext;
        string clientPV = _gatewayName;

        if( isDownstream)
                {
                clientPV += ":ds:bypv";
                errContext = "PVs";
                }
            else
                {
                clientPV += ":us:bypv";
                errContext = "connected clients";
                }

        if( findTransmitRate)
                {
                clientPV += ":tx";
                errContext = "retrieve PV transmission rates to" + errContext;
                }
            else
                {
                clientPV += ":rx";
                errContext = "retrieve PV reception rates from" + errContext;
                }

        Value dataRates;
        
        if( ! getPVData( clientPV, errContext, dataRates))
                return false;

        Value rateDetails = dataRates["value"];

        shared_array<const string> pvName = rateDetails["name"].as<shared_array<const string>>();
        unsigned long numPVs = pvName.size();

        shared_array<const double> rate = rateDetails["rate"].as<shared_array<const double>>();
        unsigned long numRates = rate.size();

        if( numPVs != numRates)
                {
                fprintf( stdout, "        Software Error: PV data rate details are inconsistent. (%lu hosts, %lu speeds)\n", numPVs, numRates);
                return false;
                }

        for( unsigned int i = 0; i < numPVs; i++)
                if( isDownstream)
                        if( findTransmitRate)
                                fprintf( stdout, "            Gateway forwards %.4g bytes/second of %s data to clients\n", rate[i], pvName[i].c_str());
                            else
                                fprintf( stdout, "            Gateway requests %.4g bytes/second of %s data\n", rate[i], pvName[i].c_str());
                    else
                        if( findTransmitRate)
                                fprintf( stdout, "            Gateway sends %.4g bytes/second to request %s data\n", rate[i], pvName[i].c_str());
                            else
                                fprintf( stdout, "            Gateway receives %.4g bytes/second of %s data\n", rate[i], pvName[i].c_str());

        return true;
        }

std::string GatewayStatus::
convertFromIPAddr( std::string ipAddr)
        {
        struct sockaddr_in saddr;
        string::size_type pos;
        string ip = ipAddr;
        string hostName;

        pos = ipAddr.find( ":");
        if( pos != string::npos)
                ip = ipAddr.substr( 0, pos);

        saddr.sin_family = AF_INET;
        saddr.sin_port = htons( 80);  // arbitrary
        saddr.sin_addr.s_addr = inet_addr( ip.c_str());

        char host[NI_MAXHOST];

        getnameinfo( reinterpret_cast<struct sockaddr *>( &saddr), sizeof saddr, host, sizeof host, nullptr, 0, NI_NOFQDN);
        return hostName = host;
        }

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
                unsigned int setGatewayOptions( int argc, char *const argv[]);

                bool showPVConnections();
                bool showTransferRates();
                bool showIOCConnections();

                std::string convertFromIPAddr( std::string ipAddr);
        private:
                void showHelp( string appName);
                bool showRPCResults( string pvName);
                bool showPermissions( pvxs::Value result);
                bool showPVRates( bool isDownstream, bool findTransmitRate);
                int checkOptionValue( const string value, const string mesg);
                bool showHostRates( bool isDownstream, bool findTransmitRate);
                bool getPVData( string pvName, string errContext, pvxs::Value &pvData);

                string _gatewayName;

                unsigned int _maxAll;
                unsigned int _maxPVs;
                unsigned int _maxPeers;

                bool _showRequestRates;
                client::Context _context;
        };

int
main( int argc, char *const argv[])
        {
        unsigned int i;
        unsigned int nargs = argc;

        GatewayStatus status;

        for( i = status.setGatewayOptions( argc, argv); i < nargs; i++)
                {
                status.setGatewayName( argv[i]);

                if( ! status.showIOCConnections())
                        continue;

                (void)status.showPVConnections();
                (void)status.showTransferRates();
                }

        return 0;
        }

unsigned int GatewayStatus::
setGatewayOptions( int argc, char *const argv[])
        {
        int c;
        extern int optopt;
        extern int opterr;

        opterr = 0;
	while (( c = getopt( argc, argv, ":c:hm:p:r")) != -1)
                {
		switch (c)
                        {

                case 'm':
                        _maxAll = checkOptionValue( optarg, "Missing value to limit number of PVs or computer names in report.");
                        break;

                case 'p':
                        _maxPVs = checkOptionValue( optarg, "Missing value to limit number of PVs in output.");
                        break;

                case 'r':
                        _showRequestRates = true;
                        break;

                case 'c':
                        _maxPeers = checkOptionValue( optarg, "Missing value to limit number of computer names in output.");
                        break;

                case ':':
                        fprintf( stderr, "Missing value for '%c' option.\n\n", optopt);
                        exit( 1);
                        break;

		default:
                        optopt = c;
                        /* fall through */
                case '?':
			fprintf( stderr, "Unrecognized option (\"%c\")\n\n", optopt);
                        /* fall through */
		case 'h':
                        showHelp( argv[0]);
                        break;
                        }
                }

        return optind;
        }

int GatewayStatus::
checkOptionValue( const string value, const string mesg)
        {

        if( ! isdigit( value[0]))
                {
                fprintf( stderr, "%s\n", mesg.c_str());
                exit( 1);
                }
        return atoi( value.c_str());
        }

void GatewayStatus::
showHelp( string appName)
        {

        fprintf( stdout, "Print status information about EPICS 7 (pvAccess) gateways.\n");
        fprintf( stdout, "Usage: %s [OPTIONS] gateway-name-prefix ...\n", appName.c_str());
        fprintf( stdout, "       The 'gateway-name-prefix' is the value of the gateway's\n");
        fprintf( stdout, "       'statusprefix' entry in its configuration file.\n");
        fprintf( stdout, "The following options are supported:\n");
        fprintf( stdout, "  -h   print this message then exit.\n");
        fprintf( stdout, "  -r   Include the rates measured for requests, both coming\n");
        fprintf( stdout, "       into the gateway, and then from the gateway to IOCs\n");
        fprintf( stdout, "  -m N Report only the top 'N' transfer rates for PVs and Computers\n");
        fprintf( stdout, "  -c N Report only the top 'N' transfer rates for Computers\n");
        fprintf( stdout, "  -p N Report only the top 'N' transfer rates for PVs\n");
        exit( 0);
        }

GatewayStatus::
GatewayStatus() :
_context( client::Context::fromEnv())
        {

        _maxAll = 1000000;
        _maxPVs = 1000000;
        _maxPeers = 1000000;
        _showRequestRates = false;
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
showTransferRates()
        {

        fprintf( stdout, "    Client and IOC Transfer Rates :\n");

        fprintf( stdout, "        Gateway's Server Side:\n");
        (void)showHostRates( true, true);
        if( _showRequestRates)
                (void)showHostRates( true, false);

        fprintf( stdout, "        Gateway's Client Side:\n");
        (void)showHostRates( false, false);
        if( _showRequestRates)
                (void)showHostRates( false, true);

        fprintf( stdout, "    PV Transfer Rates:\n");

        fprintf( stdout, "        Gateway's Server Side:\n");
        (void)showPVRates( true, true);
        if( _showRequestRates)
                (void)showPVRates( true, false);

        fprintf( stdout, "        Gateway's Client Side:\n");
        (void)showPVRates( false, false);
        if( _showRequestRates)
                (void)showPVRates( false, true);

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

unsigned int
findMin( unsigned int a, unsigned int b, unsigned int c)
        {

        if( a < b)
                {
                if( a < c)
                        return a;
                return c;
                }
        if( b < c)
                return b;
        return c;
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

        unsigned int limit = findMin( sizeHost, _maxPeers, _maxAll);
        for( unsigned int i = 0; i < limit; i++)
                {
                string hostName = convertFromIPAddr( host[i]);

                if( findTransmitRate)
                        {
                        if( isDownstream)
                                fprintf( stdout, "            Gateway transmits %9.4f bytes/second  to  %s on %s\n", rate[i], user[i].c_str(), hostName.c_str());
                            else
                                fprintf( stdout, "            Gateway transmits %9.4f bytes/second  to  %s\n", rate[i], hostName.c_str());
                        }
                    else
                        {
                        if( isDownstream)
                                fprintf( stdout, "            Gateway receives  %9.4f bytes/second from %s on %s\n", rate[i], user[i].c_str(), hostName.c_str());
                            else
                                fprintf( stdout, "            Gateway receives  %9.4f bytes/second from %s\n", rate[i], hostName.c_str());
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

        unsigned int limit = findMin( numPVs, _maxPVs, _maxAll);
        for( unsigned int i = 0; i < limit; i++)
                if( isDownstream)
                        if( findTransmitRate)
                                fprintf( stdout, "            Gateway forwards  %9.4f bytes/second of %s data to clients\n", rate[i], pvName[i].c_str());
                            else
                                fprintf( stdout, "            Gateway requests  %9.4f bytes/second of %s data\n", rate[i], pvName[i].c_str());
                    else
                        if( findTransmitRate)
                                fprintf( stdout, "            Gateway sends     %9.4f bytes/second to request %s data\n", rate[i], pvName[i].c_str());
                            else
                                fprintf( stdout, "            Gateway receives  %9.4f bytes/second of %s data\n", rate[i], pvName[i].c_str());

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

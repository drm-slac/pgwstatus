/*
 * display EPICS p4p gateway diagnostic PVs
 */
#include <string>
#include <iostream>

#include <pvxs/client.h>

using namespace pvxs;
using namespace std;

std::string convertFromIPAddr( std::string ipAddr);
bool showPVConnections( client::Context, string gatewayName);
bool showIOCConnections( client::Context, string gatewayName);
bool showRPCResults( client::Context, string, string gatewayName);
bool showPermissions( client::Context context, string gatewayName, pvxs::Value result);
bool getPVData( client::Context context, string pvName, string errContext, pvxs::Value &pvData);
bool showPVRates( client::Context context, string gatewayName, bool isDownstream, bool findTransmitRate);
bool showHostRates( client::Context context, string gatewayName, bool isDownstream, bool findTransmitRate);

int
main( int argc, char *argv[])
        {
        unsigned int i;
        unsigned int nargs = argc;

        client::Context context( client::Context::fromEnv());

        if( nargs <= 1)
                {
                printf( "Usage: %s gateway-name ...\n", argv[0]);
                exit( 1);
                }

        for( i = 1; i < nargs; i++)
                {
                printf( "gateway %s:\n", argv[i]);

                if( ! showIOCConnections( context, argv[i]))
                        continue;

                (void)showPVConnections( context, argv[i]);

                printf( "    Client and IOC Transfer Rates :\n");
                (void)showHostRates( context, argv[i], false, false);
                (void)showHostRates( context, argv[i], true, true);
                (void)showHostRates( context, argv[i], true, false);
                (void)showHostRates( context, argv[i], false, true);

                printf( "    PV Transfer Rates:\n");
                (void)showPVRates( context, argv[i], false, false);
                (void)showPVRates( context, argv[i], true, true);
                (void)showPVRates( context, argv[i], true, false);
                (void)showPVRates( context, argv[i], false, true);
                }

        context.close();
        return 0;
        }

bool
showIOCConnections( client::Context context, string gatewayName)
        {
        Value reply;
        unsigned int i;
        string clientPV = gatewayName + ":clients";

        if( ! getPVData( context, clientPV, "find IOC connections", reply))
                return false;

        shared_array<const string> val = reply["value"].as<shared_array<const string>>();

        if( val.size() > 0)
                printf( "    Access to %lu IOCs:\n", val.size());
            else
                printf( "    No IOCs currently accessed\n");

        for( i = 0; i < val.size(); i++)
                {
                string host = convertFromIPAddr( val[i]);
                printf( "        %s\n", host.c_str());
                }
        return true;
        }

bool
showPVConnections( client::Context context, string gatewayName)
        {
        Value reply;
        unsigned int i;
        string clientPV = gatewayName + ":cache";

        if( ! getPVData( context, clientPV, "find PV data", reply))
                return false;

        shared_array<const string> val = reply["value"].as<shared_array<const string>>();

        if( val.size() > 0)
                printf( "    Access to %lu PVs:\n", val.size());
            else
                printf( "    No PVs currently accessed\n");
        for( i = 0; i < val.size(); i++)
                {
                string pvName = val[i];
                printf( "        %s", pvName.c_str());
                printf( "\n");
                showRPCResults( context, gatewayName, pvName);
                }

        return true;
        }

bool
showRPCResults( client::Context context, string gatewayName, string pvName)
        {
        Value reply;
        string clientPV = gatewayName + ":asTest";

        try
                {
                Value result = context.rpc( clientPV.c_str()).arg( "pv", pvName).exec()->wait( 5.0);
                reply = result;
                }
            catch( client::Timeout &)
                {
                printf( "       Cannot access gateway '%s' for details on '%s'\n", gatewayName.c_str(), pvName.c_str());
                return false;
                }
            catch( ...)
                {
                printf( "       Cannot retrieve details for %s'\n", pvName.c_str());
                return false;
                }

        if( ! reply.valid())
                {
                printf( "       No details available for '%s' from gateway '%s'\n", pvName.c_str(), gatewayName.c_str());
                return false;
                }

        string pv = reply["pv"].as<string>();
        if( pv != pvName)
                printf( "       <<Software Error>> Requested '%s' details but found '%s'.  Attempting to continue.\n", pvName.c_str(), pv.c_str());

        printf( "            requested by: %s on %s\n", reply["account"].as<string>().c_str(), convertFromIPAddr( reply["peer"].as<string>()).c_str());
        printf( "            ASG applied: %s (ASL %d)\n", reply["asg"].as<string>().c_str(), reply["asl"].as<int32_t>());

        showPermissions( context, gatewayName, reply);
        return true;
        }

bool
showPermissions( client::Context context, string gatewayName, pvxs::Value fields)
        {
        Value perms = fields["permission"];

        printf( "            PUT operations are %sallowed\n", perms["put"].as<bool>() ? "" : "NOT ");
        printf( "            RPC operations are %sallowed\n", perms["rpc"].as<bool>() ? "" : "NOT ");

        //for( Value fld : perms.ichildren())
                //{
                //std::cout << "*** FOUND " << perms.nameOf( fld) <<" : "<<fld<<"\n";
                //}
        return true;
        }

bool
showHostRates( client::Context context, string gatewayName, bool isDownstream, bool findTransmitRate)
        {
        string errContext;
        string clientPV = gatewayName;

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
        
        if( ! getPVData( context, clientPV, errContext, dataRates))
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
                printf( "        Software Error: data rate details are inconsistent. (%lu hosts, %lu speeds)\n", sizeHost, sizeRate);
                return false;
                }

        for( unsigned int i = 0; i < sizeHost; i++)
                {
                string hostName = convertFromIPAddr( host[i]);

                if( findTransmitRate)
                        {
                        if( isDownstream)
                                printf( "        Gateway transmits %.4g bytes/second  to  %s on %s\n", rate[i], user[i].c_str(), hostName.c_str());
                            else
                                printf( "        Gateway transmits %.4g bytes/second  to  %s\n", rate[i], hostName.c_str());
                        }
                    else
                        {
                        if( isDownstream)
                                printf( "        Gateway receives  %.4g bytes/second from %s on %s\n", rate[i], user[i].c_str(), hostName.c_str());
                            else
                                printf( "        Gateway receives  %.4g bytes/second from %s\n", rate[i], hostName.c_str());
                        }
                }

        return true;
        }

bool
showPVRates( client::Context context, string gatewayName, bool isDownstream, bool findTransmitRate)
        {
        string errContext;
        string clientPV = gatewayName;

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
        
        if( ! getPVData( context, clientPV, errContext, dataRates))
                return false;

        Value rateDetails = dataRates["value"];

        shared_array<const string> pvName = rateDetails["name"].as<shared_array<const string>>();
        unsigned long numPVs = pvName.size();

        shared_array<const double> rate = rateDetails["rate"].as<shared_array<const double>>();
        unsigned long numRates = rate.size();

        if( numPVs != numRates)
                {
                printf( "        Software Error: PV data rate details are inconsistent. (%lu hosts, %lu speeds)\n", numPVs, numRates);
                return false;
                }

        for( unsigned int i = 0; i < numPVs; i++)
                if( isDownstream)
                        if( findTransmitRate)
                                printf( "        Gateway forwards %.4g bytes/second of %s data to clients\n", rate[i], pvName[i].c_str());
                            else
                                printf( "        Client requests for %s data amount to %.4g bytes/second\n", pvName[i].c_str(), rate[i]);
                    else
                        if( findTransmitRate)
                                printf( "        Gateway sends %.4g bytes/second to request %s data\n", rate[i], pvName[i].c_str());
                            else
                                printf( "        Gateway receives %.4g bytes/second of %s data\n", rate[i], pvName[i].c_str());

        return true;
        }

std::string
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

bool
getPVData( client::Context context, string pvName, string errContext, pvxs::Value &pvData)
        {

        try
                {
                Value result = context.get( pvName.c_str()).exec()->wait( 5.0);
                pvData = result;
                }
            catch( client::Timeout &)
                {
                printf( "    Attempt to %s could not be completed.\n", errContext.c_str());
                return false;
                }

        if( ! pvData.valid())
                {
                printf( "    Failed to %s\n", errContext.c_str());
                return false;
                }

        return true;
        }

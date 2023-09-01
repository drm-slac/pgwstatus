# PVA Gateway Status

## Purpose

This software displays the status of a running EPICS 7 (PVAccess) Gateway.

## Command Line Options

Usage: pgwstatus [OPTIONS] gateway-name-prefix ...

> [!IMPORTANT]
> The 'gateway-name-prefix' is the value of the gateway's
*statusprefix* entry in its configuration file.

The following options are supported:

	-h   print this message then exit.
	-r   Include the rates measured for requests,both coming
		 into the gateway, and then from the gateway to IOCs
	-m N Report only the top 'N' transfer rates for PVs and
		 Computers
	-c N Report only the top 'N' transfer rates for Computers
	-p N Report only the top 'N' transfer rates for PVs

## Details

The *pgwstatus* software uses the **pvxs** library.

It makes use of several PVs created within the **p4p gateway** software, including:

* asTest
* clients
* cache
* ds:bypv:rx
* ds:bypv:tx
* us:bypv:rx
* us:bypv:tx
* ds:byhost:rx
* ds:byhost:tx
* us:byhost:rx
* us:byhost:tx

It requires
* C++11 or later compiler
* PVXS R1.2.0 or later
* EPICS 7 or later

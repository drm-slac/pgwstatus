TOP=..

include $(TOP)/configure/CONFIG

PROD_HOST += pgwstatus
pgwstatus_SRCS += pgwstatus.cpp
pgwstatus_LIBS	+= $(EPICS_BASE_HOST_LIBS)

PROD_LIBS += pvxs pvAccessCA pvAccess pvData ca Com

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


# Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

# Directories to build, any order
DIRS += configure
DIRS += src

# The build order is controlled by these dependency rules:

# All dirs except configure depend on configure
$(foreach dir, $(filter-out configure, $(DIRS)), \
    $(eval $(dir)_DEPEND_DIRS += configure))

# Add any additional dependency rules here:

include $(TOP)/configure/RULES_TOP





#DM -----------------------------------------------------
#DM # Makefile at top of application tree
#DM TOP = .
#DM include $(TOP)/configure/CONFIG
#DM 
#DM # Directories to build, any order
#DM DIRS += configure
#DM 
#DM include $(TOP)/configure/RULES_TOP
#DM 
#DM DIRS := configure
#DM 
#DM DIRS += src
#DM src_DEPEND_DIRS = configure
#DM 

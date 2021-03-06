# -*- mode: Makefile; -*- 
#--------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
#--------------------------------------------------------------------------------
# 2014-01-17
#--------------------------------------------------------------------------------

# Please define the variables below for your system!

OMI_DIR := $(shell cd ../omi-1.0.8; pwd)
BASE_DIR := $(shell pwd)
TARGET_DIR := $(BASE_DIR)/target

include $(BASE_DIR)/config.mak

PAL_OUTPUT_DIR := $(PAL_DIR)/target/$(BUILD_CONFIGURATION)

ifndef ENABLE_DEBUG
$(error "ENABLE_DEBUG is not set.  Please re-run configure")
endif

SHARED_FLAGS := -shared
INCLUDE_FLAGS := -I$(OMI_DIR)/output/include -I$(BASE_DIR)/PythonHelper
LINK_LIBRARIES := -L$(OMI_DIR)/output/lib -lmicxx -lrt
PYTHON_CFLAGS = $(shell python-config --cflags)
BUILD_SHARED_LIBRARY := g++ $(SHARED_FLAGS) $(INCLUDE_FLAGS) $(LINK_LIBRARIES) $(PYTHON_CFLAGS) -fPIC 
BUILD_OBJECT_FILE := g++ -c $(INCLUDE_FLAGS) $(PYTHON_CFLAGS) -fPIC 



PROVIDER_LIST := nxFile \
		 nxScript \
		 nxUser \
		 nxGroup \
		 nxService

PROVIDER_LIST_OMI := $(addsuffix Resource,$(addprefix MSFT_,$(PROVIDER_LIST)))
PROVIDER_LIBRARIES := $(addsuffix Resource.so,$(addprefix libMSFT_,$(PROVIDER_LIST)))
PROVIDER_LIBRARIES_PATHS := $(addprefix $(TARGET_DIR)/,$(PROVIDER_LIBRARIES))

all: $(PROVIDER_LIBRARIES_PATHS)

$(TARGET_DIR): $(TARGET_DIR)/objs

$(TARGET_DIR)/objs:
	mkdir -p $(TARGET_DIR)/objs

# PythonHelper static library
PYTHONHELPER_SRC := PythonHelper.cpp 
PYTHONHELPER_SRC_PATHS := $(addprefix $(BASE_DIR)/PythonHelper/,$(PYTHONHELPER_SRC))

PYTHONHELPER_OBJS := $(patsubst %.cpp,%.o,$(PYTHONHELPER_SRC))
PYTHONHELPER_OBJS_PATHS := $(addprefix $(TARGET_DIR)/objs/,$(PYTHONHELPER_OBJS))

$(TARGET_DIR)/objs/%.o: $(BASE_DIR)/PythonHelper/%.cpp
	$(BUILD_OBJECT_FILE) $^ -o $@

$(TARGET_DIR)/libpythonhelper.a: $(PYTHONHELPER_OBJS_PATHS)
	ar rcs $@ $^

# General Provider stuff
$(PROVIDER_LIBRARIES_PATHS): $(TARGET_DIR)/libpythonhelper.a

# nxFile
NXFILE_SRC :=   MSFT_nxFileResource.cpp \
	        MSFT_nxFileResource.h \
		module.c \
		schema.c

NXFILE_SRC_PATHS := $(addprefix $(BASE_DIR)/nxFile/,$(NXFILE_SRC))

$(NXFILE_SRC_PATHS): $(TARGET_DIR)

$(TARGET_DIR)/libMSFT_nxFileResource.so: $(NXFILE_SRC_PATHS)
	$(BUILD_SHARED_LIBRARY) $^ -o $@

# nxScript
NXSCRIPT_SRC := MSFT_nxScriptResource.cpp \
	        MSFT_nxScriptResource.h \
		module.c \
		schema.c

NXSCRIPT_SRC_PATHS := $(addprefix $(BASE_DIR)/nxScript/,$(NXSCRIPT_SRC))

$(NXSCRIPT_SRC_PATHS): $(TARGET_DIR)

$(TARGET_DIR)/libMSFT_nxScriptResource.so: $(NXSCRIPT_SRC_PATHS)
	$(BUILD_SHARED_LIBRARY) $^ -o $@

# nxUser
NXUSER_SRC := MSFT_nxUserResource.cpp \
	        MSFT_nxUserResource.h \
		module.c \
		schema.c

NXUSER_SRC_PATHS := $(addprefix $(BASE_DIR)/nxUser/,$(NXUSER_SRC))

$(NXUSER_SRC_PATHS): $(TARGET_DIR)

$(TARGET_DIR)/libMSFT_nxUserResource.so: $(NXUSER_SRC_PATHS)
	$(BUILD_SHARED_LIBRARY) $^ -o $@

# nxGroup
NXGROUP_SRC := MSFT_nxGroupResource.cpp \
	        MSFT_nxGroupResource.h \
		module.c \
		schema.c

NXGROUP_SRC_PATHS := $(addprefix $(BASE_DIR)/nxGroup/,$(NXGROUP_SRC))

$(NXGROUP_SRC_PATHS): $(TARGET_DIR)

$(TARGET_DIR)/libMSFT_nxGroupResource.so: $(NXGROUP_SRC_PATHS)
	$(BUILD_SHARED_LIBRARY) $^ -o $@

# nxService
NXSERVICE_SRC := MSFT_nxServiceResource.cpp \
	        MSFT_nxServiceResource.h \
		module.c \
		schema.c

NXSERVICE_SRC_PATHS := $(addprefix $(BASE_DIR)/nxService/,$(NXSERVICE_SRC))

$(NXSERVICE_SRC_PATHS): $(TARGET_DIR)

$(TARGET_DIR)/libMSFT_nxServiceResource.so: $(NXSERVICE_SRC_PATHS)
	$(BUILD_SHARED_LIBRARY) $^ -o $@


# nxEnvironment
$(TARGET_DIR)/libMSFT_nxEnvironmentResource.so: $(NXENVIRONMENT_SRC_PATHS)
	$(BUILD_SHARED_LIBRARY) $^ -o $@


clean :
	-rm -r $(TARGET_DIR)


include $(OMI_DIR)/output/config.mak
OMI_BINDIR := $(CONFIG_BINDIR)
OMI_LIBDIR := $(CONFIG_LIBDIR)
OMIGEN := $(OMI_BINDIR)/omigen
OMIREG := $(OMI_BINDIR)/omireg
DSC_SCHEMA_DIR := $(CONFIG_SYSCONFDIR)/dsc/configuration/schema
DSC_REGISTRATION_DIR := $(CONFIG_SYSCONFDIR)/dsc/configuration/registration

reg :
	cp -R $(BASE_DIR)/Scripts $(OMI_LIBDIR)
	for prov in $(PROVIDER_LIST); do \
		export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(OMI_LIBDIR); $(OMIREG) -n "root/Microsoft/DesiredStateConfiguration" -o @requestor@ $(TARGET_DIR)/libMSFT_$${prov}Resource.so; \
		mkdir -p  $(DSC_SCHEMA_DIR)/MSFT_$${prov}Resource; \
		mkdir -p  $(DSC_REGISTRATION_DIR)/MSFT_$${prov}Resource; \
		cp $(BASE_DIR)/$${prov}/MSFT_$${prov}Resource.schema.mof $(DSC_SCHEMA_DIR)/MSFT_$${prov}Resource/MSFT_$${prov}Resource.schema.mof; \
		cp $(BASE_DIR)/$${prov}/MSFT_$${prov}Resource.registration.mof $(DSC_REGISTRATION_DIR)/MSFT_$${prov}Resource/MSFT_$${prov}Resource.registration.mof; \
	done


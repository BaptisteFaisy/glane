# Glane — buildfile QNX (template) pour glaned
#
# Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
#
# Buildfile au format QNX (qmacros/qtargets) pour compiler glaned dans un arbre
# SDP. À VALIDER avec le SDP 8.0 réel : la syntaxe exacte des variantes et des
# chemins peut différer ; pour un premier port, préférer qnx/build.sh (qcc direct).
#
# Utilisation typique (dans un arbre de build QNX) :
#   make -C image CPULIST=x86_64
LIST=CPU

include $(MKFILES_ROOT)/qmacros.mk

# Racine du projet (à définir lors de l'appel : PROJECT_ROOT=/chemin/vers/glane)
EXTRA_SRCVPATH = $(PROJECT_ROOT)/core/src \
                 $(PROJECT_ROOT)/publish/qnx \
                 $(PROJECT_ROOT)/target

EXTRA_INCVPATH = $(PROJECT_ROOT)/core/include \
                 $(PROJECT_ROOT)/publish/qnx

# glaned + socle core/ + couche publication.
OBJS = glaned.o \
       glane_publish.o \
       desc.o desc_validate.o jsonscan.o \
       calib.o sample.o ring.o deadline.o schema.o \
       virtuel.o saturate.o selftest.o

# Pas de dépendance externe : le socle est autonome (libc QNX uniquement).
LIBS =

include $(MKFILES_ROOT)/qtargets.mk
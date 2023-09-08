# User Test
#------------------------------------


APP              = test
# App sources
APP_SRCS         = $(PROGRAM)
# App includes
APP_INC	         = . 
# Compiler flags
APP_CFLAGS       =
# Linker flags
APP_LDFLAGS      =

# Custom linker
APP_LINK_SCRIPT  =


include $(RULES_DIR)/pmsis_rules.mk

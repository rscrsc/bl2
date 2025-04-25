include $(TOPDIR)/rules.mk
 
# Name, version and release number
# The name and version of your package are used to define the variable to point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=bl2
PKG_VERSION:=1.0
PKG_RELEASE:=1
 
# Source settings (i.e. where to find the source codes)
# This is a custom variable, used below
SOURCE_DIR:=/home/rsc/openwrt/rsc/net/bl2
 
include $(INCLUDE_DIR)/package.mk
 
# Package definition; instructs on how and where our package will appear in the overall configuration menu ('make menuconfig')
define Package/bl2
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=+libstdcpp +libnetfilter-queue
  TITLE:=bl2
endef
 
# Package description; a more verbose description on what our package does
define Package/bl2/description
  Developed for research purpose.
endef
 
# Package preparation instructions; create the build directory and copy the source code. 
# The last command is necessary to ensure our preparation instructions remain compatible with the patching system.
define Build/Prepare
		mkdir -p $(PKG_BUILD_DIR)
		cp $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
		$(Build/Patch)
endef
 
# Package build instructions; invoke the target-specific compiler to first compile the source file, and then to link the file into the final executable
define Build/Compile
		$(TARGET_CXX) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/main.o -c $(PKG_BUILD_DIR)/main.cpp
		$(TARGET_CXX) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/NFQueue.o -c $(PKG_BUILD_DIR)/NFQueue.cpp
		$(TARGET_CXX) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/Faker.o -c $(PKG_BUILD_DIR)/Faker.cpp
		$(TARGET_CXX) $(TARGET_LDFLAGS) -o $(PKG_BUILD_DIR)/bl2-main $(PKG_BUILD_DIR)/*.o -lnetfilter_queue -lnfnetlink
endef
 
# Package install instructions; create a directory inside the package to hold our executable, and then copy the executable we built previously into the folder
define Package/bl2/install
		$(INSTALL_DIR) $(1)/usr/bin
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/bl2 $(1)/usr/bin
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/bl2-setup $(1)/usr/bin
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/bl2-reset $(1)/usr/bin
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/bl2-main $(1)/usr/bin
endef
 
# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,bl2))

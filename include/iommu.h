#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <sysinfo.h>
#include <shell.h>

void check_vt_support();
bool check_iommu();
void enable_iommu();
void configure_iommu();
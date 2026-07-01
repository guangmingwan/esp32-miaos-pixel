idf_component_register(
  SRCS "main.c" "mia_host_abi.c" "mia_elf_runner.c"
  INCLUDE_DIRS "."
  REQUIRES esp_driver_sdspi esp_driver_spi fatfs elf_loader
)

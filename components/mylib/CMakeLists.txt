idf_component_register(SRCS "src/fft.c"
                            "src/mic.c"
                            "src/tuner.c"
                            "src/filters.c"
                            "src/yin.c"
                            "src/utils.c"
                            "src/test.c"   # Arquivos de implementação
                    REQUIRES driver
                    REQUIRES esp_timer                    
                    INCLUDE_DIRS "include" # Diretório com os cabeçalhos
)
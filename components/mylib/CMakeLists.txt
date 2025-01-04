idf_component_register(SRCS "src/fft.c"  "src/mic.c" "src/tuner.c" "src/filters.c" "src/yin.c"   # Arquivos de implementação
                    REQUIRES driver                    
                    INCLUDE_DIRS "include" # Diretório com os cabeçalhos
)
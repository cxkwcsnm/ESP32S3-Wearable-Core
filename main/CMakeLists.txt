idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES Middlewares esp_adc)  

add_compile_options(-fdiagnostics-color=always)
                    

idf_component_register(
    SRCS "main.cpp" "WifiTimeLib.cpp"
    INCLUDE_DIRS "../include"
    REQUIRES arduino TFT_eSPI WiFiManager
)

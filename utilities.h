/*
 * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
 * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
 * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
 * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
 */

  #define NRST        5
  #define ETH_PHY_TYPE        ETH_PHY_LAN8720
  #define ETH_PHY_ADDR         0
  #define ETH_PHY_MDC         23
  #define ETH_PHY_MDIO        18
  #define ETH_PHY_POWER       -1
  #define ETH_CLK_MODE        ETH_CLOCK_GPIO17_OUT


static bool eth_connected = false;

#include "stm32g4_hal.h"

#if defined(STM32G4xx) && !defined(ARDUINO_B_G431B_ESC1)

#include "../../../../communication/SimpleFOCDebug.h"

#define SIMPLEFOC_STM32_DEBUG


// pointer to the ADC handles used in the project
ADC_HandleTypeDef hadc[ADC_COUNT] = {0};

ADC_HandleTypeDef* _get_adc_handles(){
  return hadc;
}

/**
 * Function initializing the ADC for the regular channels for the low-side current sensing
 * 
 * @param adc_instance - ADC instance to initialize
 * 
 * @return int - 0 if success 
 */
int _adc_init_regular(ADC_TypeDef* adc_instance){
  ADC_InjectionConfTypeDef sConfigInjected;
  
  
  if(adc_instance == ADC1) {
#ifdef __HAL_RCC_ADC1_CLK_ENABLE
    __HAL_RCC_ADC1_CLK_ENABLE();
#endif
#ifdef __HAL_RCC_ADC12_CLK_ENABLE
    __HAL_RCC_ADC12_CLK_ENABLE();
#endif
  }
#ifdef ADC2
  else if (adc_instance == ADC2) {
#ifdef __HAL_RCC_ADC2_CLK_ENABLE
    __HAL_RCC_ADC2_CLK_ENABLE();
#endif
#ifdef __HAL_RCC_ADC12_CLK_ENABLE
    __HAL_RCC_ADC12_CLK_ENABLE();
#endif
  }
#endif
#ifdef ADC3
  else if (adc_instance == ADC3) {
#ifdef __HAL_RCC_ADC3_CLK_ENABLE
    __HAL_RCC_ADC3_CLK_ENABLE();
#endif
#ifdef __HAL_RCC_ADC34_CLK_ENABLE
    __HAL_RCC_ADC34_CLK_ENABLE();
#endif
#if defined(ADC345_COMMON)
    __HAL_RCC_ADC345_CLK_ENABLE();
#endif
  } 
#endif
#ifdef ADC4
  else if (adc_instance == ADC4) {
#ifdef __HAL_RCC_ADC4_CLK_ENABLE
    __HAL_RCC_ADC4_CLK_ENABLE();
#endif
#ifdef __HAL_RCC_ADC34_CLK_ENABLE
    __HAL_RCC_ADC34_CLK_ENABLE();
#endif
#if defined(ADC345_COMMON)
    __HAL_RCC_ADC345_CLK_ENABLE();
#endif
  }
#endif
#ifdef ADC5
  else if (adc_instance == ADC5) {
#if defined(ADC345_COMMON)
    __HAL_RCC_ADC345_CLK_ENABLE();
#endif
  }
#endif
  else{
#ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: Cannot find a common ADC for the pins!");
#endif
    return -1; // error not a valid ADC instance
  }

  int adc_num = _adcToIndex(adc_instance);

#ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: Using ADC: ", adc_num+1);
#endif

  hadc[adc_num].Instance = adc_instance;

  hadc[adc_num].Init.ClockPrescaler =  ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc[adc_num].Init.Resolution = ADC_RESOLUTION_12B;
  hadc[adc_num].Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc[adc_num].Init.ContinuousConvMode = DISABLE;
  hadc[adc_num].Init.LowPowerAutoWait = DISABLE;
  hadc[adc_num].Init.GainCompensation = 0;
  hadc[adc_num].Init.DiscontinuousConvMode = DISABLE;
  hadc[adc_num].Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc[adc_num].Init.ExternalTrigConv = ADC_SOFTWARE_START; // for now
  hadc[adc_num].Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc[adc_num].Init.NbrOfConversion = 1;
  hadc[adc_num].Init.DMAContinuousRequests = DISABLE;
  hadc[adc_num].Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc[adc_num].Init.Overrun = ADC_OVR_DATA_PRESERVED;
  if ( HAL_ADC_Init(&hadc[adc_num]) != HAL_OK){
#ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot init ADC!");
#endif
    return -1;
  }
  
  return 0;
}

/**
 * Function initializing the ADC and the injected channels for the low-side current sensing
 * 
 * @param cs_params - current sense parameters
 * @param driver_params - driver parameters
 * 
 * @return int - 0 if success 
 */
int _adc_init(Stm32CurrentSenseParams* cs_params, const STM32DriverParams* driver_params)
{

  /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
  */
  auto adc_instance = _findBestADCForInjectedPins(3, cs_params->pins, hadc);
  if(adc_instance == NP){
#ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: No ADCs available!");
#endif
    return -1; // error not a valid ADC instance
  }
  if( _adc_init_regular(adc_instance) != 0){
    return -1;
  }
  int adc_num = _adcToIndex(adc_instance);

  /**Configures for the selected ADC injected channel its corresponding rank in the sequencer and its sample time 
    */
  ADC_InjectionConfTypeDef sConfigInjected;
  sConfigInjected.InjectedNbrOfConversion = 0;
  for(int pin_no=0; pin_no<3; pin_no++){
    if(_isset(cs_params->pins[pin_no])){
      sConfigInjected.InjectedNbrOfConversion++;
    }
  }
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;  
  sConfigInjected.AutoInjectedConv = DISABLE;
  sConfigInjected.InjectedSingleDiff = ADC_SINGLE_ENDED;
  sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected.InjectedOffsetNumber = ADC_OFFSET_NONE;
  sConfigInjected.InjectedOffset = 0;
  sConfigInjected.InjecOversamplingMode = DISABLE;
  sConfigInjected.QueueInjectedContext = DISABLE;

  // automating TRGO flag finding - hardware specific
  uint8_t tim_num = 0;
  while(driver_params->timers_handle[tim_num] != NP && tim_num < 6){
    uint32_t trigger_flag = _timerToInjectedTRGO(driver_params->timers_handle[tim_num++]);
    if(trigger_flag == _TRGO_NOT_AVAILABLE) continue; // timer does not have valid trgo for injected channels

    // check if TRGO used already - if yes use the next timer
    if((driver_params->timers_handle[tim_num-1]->Instance->CR2 & LL_TIM_TRGO_ENABLE) || // if used for timer sync
       (driver_params->timers_handle[tim_num-1]->Instance->CR2 & LL_TIM_TRGO_UPDATE)) // if used for ADC sync
      {
      continue;
    }

    // if the code comes here, it has found the timer available
    // timer does have trgo flag for injected channels  
    sConfigInjected.ExternalTrigInjecConv = trigger_flag;
    
    // this will be the timer with which the ADC will sync
    cs_params->timer_handle = driver_params->timers_handle[tim_num-1];
    // done
    break;
  }
  if( cs_params->timer_handle == NP ){
    // not possible to use these timers for low-side current sense
#ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ERR: cannot sync any timer to injected channels!");
#endif
    return -1;
  }else{
#ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: Using timer: ", stm32_getTimerNumber(cs_params->timer_handle->Instance));
#endif
  }
  
  // ★ FIX for STM32G4: HAL_ADCEx_InjectedConfigChannel overwrites the entire
  //   JSQR register on each call. When called twice (for 2 channels), only the
  //   last channel's configuration survives. Fix: collect all channels first,
  //   then configure them ALL in a single HAL call by building the config
  //   with the proper rank assignments.
  //
  //   Alternative approach: configure rank 1 first, then rank 2 with
  //   QueueInjectedContext ENABLED so rank 2 doesn't overwrite rank 1.
  //   But simplest: just set JSQR directly via LL driver after HAL setup.
  
  // First, collect all channel numbers
  uint8_t channel_no = 0;
  uint32_t channels[4] = {0};
  int num_channels = 0;
  for(int i=0; i<3; i++){
    if (!_isset(cs_params->pins[i])) continue;
    channels[num_channels] = _getADCChannel(analogInputToPinName(cs_params->pins[i]), hadc[adc_num].Instance);
#ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: ADC ch: ", (int)channels[num_channels]);
#endif
    num_channels++;
  }
  
  if(num_channels == 0) return -1;
  
  // Configure the LAST rank first (so HAL sets up the JSQR length etc.)
  // Then patch the earlier ranks into JSQR directly.
  // Actually, the cleanest fix: configure via HAL once for all ranks,
  // but use a single sConfigInjected call with proper JSQR building.
  
  // Build the complete JSQR register value:
  //   JSQR[1:0]   = JL     (sequence length - 1)
  //   JSQR[14:9]  = JSQ1   (channel for rank 1) -- note: actual bit layout
  //   JSQR[20:15] = JSQ2   (channel for rank 2)
  //   JSQR[26:21] = JSQ3   (channel for rank 3)
  //   JSQR[31:27] = JSQ4   (channel for rank 4) -- only 5 bits!
  //   JSQR[8:7]   = JEXTEN (trigger edge)
  //   JSQR[6:2]   = JEXTSEL (trigger source)
  
  // Step 1: Let HAL configure rank 1 (this sets trigger, length, etc.)
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_1;
  sConfigInjected.InjectedChannel = channels[0];
  if (HAL_ADCEx_InjectedConfigChannel(&hadc[adc_num], &sConfigInjected) != HAL_OK){
    return -1;
  }
  
  // Step 2: For additional channels, patch JSQR register directly
  //   The HAL call above set JSQR with JL=num_channels-1 and JSQ1=channels[0].
  //   We need to add JSQ2, JSQ3, JSQ4 without overwriting JSQ1.
  if(num_channels >= 2){
    // Read current JSQR, patch in JSQ2
    uint32_t jsqr = hadc[adc_num].Instance->JSQR;
    // JSQ2 is bits [20:15] (6 bits) -- clear JSQ2 field, then set it 
    jsqr &= ~(0x1F << 15);  // clear JSQ2 (5 bits: channel number)
    jsqr |= ((__LL_ADC_CHANNEL_TO_DECIMAL_NB(channels[1])) << 15);
    hadc[adc_num].Instance->JSQR = jsqr;
#ifdef SIMPLEFOC_STM32_DEBUG
    SIMPLEFOC_DEBUG("STM32-CS: JSQR patched for 2 channels");
#endif
  }
  if(num_channels >= 3){
    uint32_t jsqr = hadc[adc_num].Instance->JSQR;
    jsqr &= ~(0x1F << 21);  // clear JSQ3
    jsqr |= ((__LL_ADC_CHANNEL_TO_DECIMAL_NB(channels[2])) << 21);
    hadc[adc_num].Instance->JSQR = jsqr;
  }
  cs_params->adc_handle = &hadc[adc_num];
  return 0;
}

/**
 * Function to initialize the ADC GPIO pins
 * 
 * @param cs_params current sense parameters
 * @param pinA pin number for phase A
 * @param pinB pin number for phase B
 * @param pinC pin number for phase C
 * @return int 0 if success, -1 if error
 */
int _adc_gpio_init(Stm32CurrentSenseParams* cs_params, const int pinA, const int pinB, const int pinC)
{
  int pins[3] = {pinA, pinB, pinC};
  const char* port_names[3] = {"A", "B", "C"};
  for(int i=0; i<3; i++){
    if(_isset(pins[i])){
      // check if pin is an analog pin
      if(pinmap_peripheral(analogInputToPinName(pins[i]), PinMap_ADC) == NP){

#if defined(SIMPLEFOC_STM32_DEBUG) && !defined(SIMPLEFOC_DISABLE_DEBUG)
        SimpleFOCDebug::print("STM32-CS: ERR: Pin ");
        SimpleFOCDebug::print(port_names[i]);
        SimpleFOCDebug::println(" does not belong to any ADC!");
#endif
        return -1;
      }
      pinmap_pinout(analogInputToPinName(pins[i]), PinMap_ADC);
      cs_params->pins[i] = pins[i];
    }
  }
  return 0;
}


extern "C" {
  void ADC1_2_IRQHandler(void)
  {
    if (hadc[0].Instance != NP)
      HAL_ADC_IRQHandler(&hadc[0]);
    if (hadc[1].Instance != NP)
      HAL_ADC_IRQHandler(&hadc[1]);
  }
#ifdef ADC3
  void ADC3_IRQHandler(void)
  {
      if (hadc[2].Instance != NP)
        HAL_ADC_IRQHandler(&hadc[2]);
  }
#endif

#ifdef ADC4
  void ADC4_IRQHandler(void)
  {
      if (hadc[3].Instance != NP)
        HAL_ADC_IRQHandler(&hadc[3]);
  }
#endif

#ifdef ADC5
  void ADC5_IRQHandler(void)
  {
      if (hadc[4].Instance != NP)
        HAL_ADC_IRQHandler(&hadc[4]);
  }
#endif
}
#endif
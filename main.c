#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

FILE *fr;
FILE *of;

union sensor_packet_t{
    uint8_t i8[4];
    int32_t  i32;
};

union CRC_packet_t{
    uint8_t u8[2];
    uint16_t u16;
};

struct data_set_t{
    uint32_t timestamp;
    uint8_t  status;
    union sensor_packet_t current_raw;
    union sensor_packet_t voltage_raw;
    float current;
    float voltage;
    union CRC_packet_t CRC_value;
};


// Compute the MODBUS RTU CRC
//Adopted from http://www.ccontrolsys.com/w/How_to_Compute_the_Modbus_RTU_Message_CRC 16.7.2015
uint16_t ModRTU_CRC(uint8_t buf[], int len)
{
  uint16_t crc = 0xFFFF;

  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];          // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;
}


int main(int argc, char **argv)
{
    errno = 0;
    char line[80];

    if(argc > 1){
        fr = fopen (argv[1], "rt");
    }
    else{
        fr = fopen ("EMData.txt", "rt");
    }

    of = fopen("EMData_parsed.csv", "w");

    struct data_set_t data_set;

    fgets(line, 80, fr);
    printf("%s", line);
    fgets(line, 80, fr);
    printf("%s", line);
    fgets(line, 80, fr);
    printf("%s", line);

    fprintf(of, "Time[ms], Current[A], Voltage[V], Power[W]\n");

    while(fgets(line, 80, fr) != NULL){
        uint16_t CRC = 0;
        uint8_t data_array[8];

        char out_buffer[256];

        sscanf (line, "%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n", \
                       &(data_set.timestamp),
                       &(data_set.status),
                       &(data_set.current_raw.i8[1]),
                       &(data_set.current_raw.i8[2]),
                       &(data_set.current_raw.i8[3]),
                       &(data_set.voltage_raw.i8[1]),
                       &(data_set.voltage_raw.i8[2]),
                       &(data_set.voltage_raw.i8[3]),
                       &(data_set.CRC_value.u8[0]),
                       &(data_set.CRC_value.u8[1])
                    );
        data_set.voltage = (data_set.voltage_raw.i32 / 256.0) / 1000.0;
        data_set.current = (data_set.current_raw.i32 / 256.0) / 1000.0;

        data_array[0] = data_set.status;
        data_array[1] = (data_set.current_raw.i8[1]);
        data_array[2] = (data_set.current_raw.i8[2]);
        data_array[3] = (data_set.current_raw.i8[3]);
        data_array[4] = (data_set.voltage_raw.i8[1]);
        data_array[5] = (data_set.voltage_raw.i8[2]);
        data_array[6] = (data_set.voltage_raw.i8[3]);

        CRC = ModRTU_CRC(data_array, 7);

        //printf("Timestamp: %d\tCurrent: %.03f\tVoltage: %.03f CRC Valid: %d\n", data_set.timestamp, data_set.current, data_set.current, CRC == data_set.CRC_value.u16);
        if(CRC == data_set.CRC_value.u16){
            fprintf(of, "%d, %.03f, %.03f, %.03f\n", data_set.timestamp, data_set.current, data_set.voltage, data_set.current * data_set.voltage);
        }
    }

    fclose(fr);
    fclose(of);
    return 0;
}




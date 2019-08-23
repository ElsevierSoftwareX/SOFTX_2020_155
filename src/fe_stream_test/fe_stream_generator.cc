//
// Created by jonathan.hanks on 8/22/19.
//
#include "fe_stream_generator.hh"

GeneratorPtr
create_generator(const std::string& generator, const SimChannel& ch)
{
  if (ch.data_type() != 2)
    throw std::runtime_error("Invalid/unsupported data type for a generator");
  if (generator == "gps_sec") {
    return GeneratorPtr(new Generators::GPSSecondGenerator(ch));
  }
  throw std::runtime_error("Unknown generator type");
}

GeneratorPtr
create_generator(const std::string& channel_name)
{
  std::vector<std::string> parts = split(channel_name, "--");
  if (parts.size() < 4)
    throw std::runtime_error("Generator name has too few parts, invalid input");
  int data_type = 0;
  {
    std::istringstream is (parts[parts.size()-2]);
    is >> data_type;
  }
  int rate = 0;
  {
    std::istringstream is (parts[parts.size()-1]);
    is >> rate;
  }
  if (!(data_type == 2 || data_type == 4) || rate < 16)
    throw std::runtime_error("Invalid data type or rate found");
  std::string& base = parts[0];
  std::string& name = parts[1];
  int arg_count = parts.size()-4; // ignore base channel name, data type, rate
  if (name == "gpssoff1p" && arg_count == 1)
  {
    std::istringstream is(parts[2]);
    int offset = 0;
    is >> offset;
    if (data_type == 2)
      return GeneratorPtr(new Generators::GPSSecondWithOffset<int>(SimChannel(base, data_type, rate, 0), offset));
    else
      return GeneratorPtr(new Generators::GPSSecondWithOffset<float>(SimChannel(base, data_type, rate, 0), offset));

  }
  else if (name == "gpssmd100koff1p" && arg_count == 1)
  {
    std::istringstream is(parts[2]);
    int offset = 0;
    is >> offset;
    if (data_type == 2)
      return GeneratorPtr(new Generators::GPSMod100kSecWithOffset<int>(SimChannel(base, data_type, rate, 0), offset));
    else
      return GeneratorPtr(new Generators::GPSMod100kSecWithOffset<float>(SimChannel(base, data_type, rate, 0), offset));
  }
  else
  {
    throw std::runtime_error("Unknown generator type");
  }
}

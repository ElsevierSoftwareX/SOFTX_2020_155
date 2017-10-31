//
// Created by jonathan.hanks on 10/31/17.
//

#ifndef DAQD_FE_STREAM_GENERATOR_HH
#define DAQD_FE_STREAM_GENERATOR_HH

#include <tr1/memory>

#include <iostream>
#include <sstream>
#include <string>

class Generator
{
protected:
    virtual int data_type() = 0;
    virtual int data_rate() = 0;
    virtual const std::string& channel_base_name() = 0;

    virtual std::string other_params() { return ""; }
public:
    virtual std::string generator_name() const = 0;
    virtual std::string full_channel_name() const
    {
        std::ostringstream os;
        os << channel_base_name() << "--" << generator_name() << "--" << data_type() << "--" << data_rate() << other_params();
        return os.str();
    }

    virtual template <typename output_iterator>
    void generate(int gps_sec, int gps_nano, output_iterator out) = 0;
};

typedef std::tr1::shared_ptr<Generator> GeneratorPtr;

namespace Generators {
    class GeneratorParams: public Generator {
    protected:
        int data_type_;
        int data_rate_;
        std::string base_name_;

        virtual int data_type() { return data_type_; };
        virtual int data_rate() { return data_rate; };
        virtual const std::string& channel_base_name() { return base_name_; };
    public:
        GeneratorParams(const std::string base_name, int data_type, int data_rate):
                data_type_(data_type), data_rate_(data_rate), base_name_(base_name) {}
    };

    class GPSSecondGenerator: public GeneratorParams
    {
    public:
        GPSSecondGenerator(const std::string base_name, int data_type, int data_rate):
        GeneratorParams(base_name, data_type, data_rate) {}

        std::string generator_name() const { return "gps_sec"; }

        virtual template <typename output_iterator>
        void generate(int gps_sec, int gps_nano, output_iterator out)
        {
            int rate = data_rate() / 16;
            for (int i = 0; i < rate; ++i)
            {
                *out = gps_sec;
                ++out;
            }
        }
    };
}

GeneratorPtr create_generator(const std::string generator, std::string& base_name, int data_type, int data_rate)
{
    if (data_type != 4)
        throw std::runtime_error("Invalid/unsupported data type for a generator");
    if (generator == "gps_sec") {
        return GeneratorPtr(new Generators::GPSSecondGenerator(base_name, date_type, data_rate));
    }
    throw std::runtime_error("Unknown generator type");
}

#endif //DAQD_FE_STREAM_GENERATOR_HH

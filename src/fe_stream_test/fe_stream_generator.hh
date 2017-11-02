//
// Created by jonathan.hanks on 10/31/17.
//

#ifndef DAQD_FE_STREAM_GENERATOR_HH
#define DAQD_FE_STREAM_GENERATOR_HH

#include <tr1/memory>

#include <iostream>
#include <sstream>
#include <string>

class SimChannel
{
private:
    std::string name_;
    int chtype_;
    int rate_;
    int chnum_;
public:
    SimChannel(): name_(""), chtype_(0), rate_(0), chnum_(0) {}
    SimChannel(std::string name, int chtype, int rate, int chnum):
            name_(name), chtype_(chtype), rate_(rate), chnum_(chnum) {}
    SimChannel(const SimChannel& other):
            name_(other.name_), chtype_(other.chtype_), rate_(other.rate_),
            chnum_(other.chnum_) {}

    SimChannel& operator=(const SimChannel& other)
    {
        if (this != &other)
        {
            name_ = other.name_;
            chtype_ = other.chtype_;
            rate_ = other.rate_;
            chnum_ = other.chnum_;
        }
        return *this;
    }

    int data_type() const { return chtype_; }
    int data_rate() const { return rate_; }
    int channel_num() const { return chnum_; }

    const std::string& name() const { return name_; }
};

class Generator
{
protected:
    virtual int data_type() const = 0;
    virtual int data_rate() const = 0;
    virtual const std::string& channel_base_name() const = 0;

    virtual std::string other_params() const { return ""; }
public:
    virtual std::string generator_name() const = 0;
    virtual std::string full_channel_name() const
    {
        std::ostringstream os;
        os << channel_base_name() << "--" << generator_name() << other_params() << "--" << data_type() << "--" << data_rate();
        return os.str();
    }

    virtual char* generate(int gps_sec, int gps_nano, char *out) = 0;

    virtual void output_ini_entry(std::ostream& os) = 0;
    virtual void output_par_entry(std::ostream& os) = 0;
};

typedef std::tr1::shared_ptr<Generator> GeneratorPtr;

namespace Generators {
    class SimChannelGenerator : public Generator {
    protected:
        SimChannel ch_;

        virtual int data_type() const { return ch_.data_type(); };
        virtual int data_rate() const { return ch_.data_rate(); };
        virtual const std::string& channel_base_name() const { return ch_.name(); };
    public:
        SimChannelGenerator(const SimChannel& ch):
                ch_(ch) {}

        virtual void output_ini_entry(std::ostream& os)
        {
            os << "[" << full_channel_name() << "]\n";
            os << "datarate=" << data_rate() << "\n";
            os << "datatype=" << data_type() << "\n";
            os << "chnnum=" << ch_.channel_num() << "\n";
        }

        virtual void output_par_entry(std::ostream& os) {}
    };

    
    
    class GPSSecondGenerator: public SimChannelGenerator
    {
    public:
        GPSSecondGenerator(const SimChannel& ch):
                SimChannelGenerator(ch) {}

        std::string generator_name() const { return "gps_sec"; }

        char* generate(int gps_sec, int gps_nano, char* out)
        {
            int rate = data_rate() / 16;
            int *out_ = reinterpret_cast<int*>(out);
            for (int i = 0; i < rate; ++i)
            {
                *out_ = gps_sec;
                ++out_;
            }
            return reinterpret_cast<char*>(out_);
        }
    };
    
    class GPSSecondWithOffset: public SimChannelGenerator
    {
        int offset_;
    public:
        GPSSecondWithOffset(const SimChannel& ch, int offset):
                SimChannelGenerator(ch), offset_(offset) {}

        std::string generator_name() const { return "gps_sec_off1p"; }

        std::string other_params() const
        {
            std::ostringstream os;
            os << "--" << offset_;
            return os.str();
        }

        char* generate(int gps_sec, int gps_nano, char* out)
        {
            int rate = data_rate() / 16;
            int *out_ = reinterpret_cast<int*>(out);
            for (int i = 0; i < rate; ++i)
            {
                *out_ = gps_sec + offset_;
                ++out_;
            }
            return reinterpret_cast<char*>(out_);
        }
    };
}

GeneratorPtr create_generator(const std::string& generator, const SimChannel& ch)
{
    if (ch.data_type() != 4)
        throw std::runtime_error("Invalid/unsupported data type for a generator");
    if (generator == "gps_sec") {
        return GeneratorPtr(new Generators::GPSSecondGenerator(ch));
    }
    throw std::runtime_error("Unknown generator type");
}

#endif //DAQD_FE_STREAM_GENERATOR_HH

#ifndef PAL_STATISTICS_MSG_H
#define PAL_STATISTICS_MSG_H

#include <string>
#include <vector>
#include <map>

#include <ros/types.h>
#include <ros/serialization.h>
#include "ros_parser_base.h"
#include <std_msgs/Header.h>


struct PalStatisticsNames_
{
//    Header header
//    string[] names
//    uint32 names_version #This is increased each time names change
    std_msgs::Header header;
    std::vector<std::string> names;
    uint32_t names_version;
};

struct PalStatisticsValues_
{
//    Header header
//    float64[] names
//    uint32 names_version #This is increased each time names change
    std_msgs::Header header;
    std::vector<double> values;
    uint32_t names_version;
};

//-----------------------------------------------------

namespace ros
{
namespace serialization
{

template<> struct Serializer< ::PalStatisticsNames_ >
{
    template<typename Stream, typename T> inline static void allInOne(Stream& stream, T m)
    {
        stream.next(m.header);
        stream.next(m.names);
        stream.next(m.names_version);
    }
    ROS_DECLARE_ALLINONE_SERIALIZER
};

template<> struct Serializer< ::PalStatisticsValues_ >
{
    template<typename Stream, typename T> inline static void allInOne(Stream& stream, T m)
    {
        stream.next(m.header);
        stream.next(m.values);
        stream.next(m.names_version);
    }
    ROS_DECLARE_ALLINONE_SERIALIZER
};

} // namespace serialization
} // namespace ros

//-----------------------------------------------------

static std::map<uint32_t, std::vector<std::string> > _stored_pal_statistics_names;

class PalStatisticsNamesParser: public RosParserBase
{
public:

    PalStatisticsNamesParser() = default;

    static const std::string& getCompatibleKey()
    {
        static std::string temp =  "bece3d42a81d5c50cd68f110cf17bf55";
        return temp;
    }

    const std::unordered_set<std::string>& getCompatibleKeys() const override
    {
        static std::unordered_set<std::string> temp = { getCompatibleKey() };
        return temp;
    }

    virtual void pushMessageRef(const std::string& ,
                                const MessageRef& msg,
                                double) override
    {
        PalStatisticsNames_ pal_names;
        ros::serialization::IStream is( const_cast<uint8_t*>(msg.data()), msg.size() );
        ros::serialization::deserialize(is, pal_names);
        _stored_pal_statistics_names.insert( {pal_names.names_version, std::move(pal_names.names) });
    }

    void extractData(PlotDataMapRef& , const std::string& ) override
    {  }
};

//-----------------------------------------------------
class PalStatisticsValuesParser: public RosParserBase
{
public:

    PalStatisticsValuesParser() = default;

    static const std::string& getCompatibleKey()
    {
        static std::string temp =  "44646896ace86f96c24fbb63054eeee8";
        return temp;
    }

    const std::unordered_set<std::string>& getCompatibleKeys() const override
    {
        static std::unordered_set<std::string> temp = { getCompatibleKey() };
        return temp;
    }

    virtual void pushMessageRef(const std::string& ,
                                const MessageRef& msg,
                                double timestamp) override
    {
        PalStatisticsValues_ pal_msg;
        ros::serialization::IStream is( const_cast<uint8_t*>(msg.data()), msg.size() );
        ros::serialization::deserialize(is, pal_msg);

        auto& values = _data[ pal_msg.names_version ];

        if( _use_header_stamp )
        {
            timestamp = pal_msg.header.stamp.toSec();
        }

        for( size_t index = 0; index < pal_msg.values.size(); index++)
        {
            if( index >= values.size() )
            {
                values.emplace_back( "placeholder" );
            }
            values[index].pushBack( { timestamp, pal_msg.values[index] } );
        }
    }

    void extractData(PlotDataMapRef& plot_map, const std::string& prefix) override
    {
        for ( auto& it_version: _data)
        {
            const auto& names = _stored_pal_statistics_names[ it_version.first ];
            auto& vect = it_version.second;
            for ( size_t index = 0; index < vect.size(); index++ )
            {
                appendData(plot_map, fmt::format("{}/{}", prefix, names.at(index) ), vect[index]);
            }
        }
    }

private:
    std::map<uint32_t, std::vector< PlotData > > _data;

};



#endif // PAL_STATISTICS_MSG_H

#include "ros_parser.h"
#include "dialog_with_itemlist.h"
#include "geometry_twist_msg.h"
#include "diagnostic_msg.h"
#include "pal_statistics_msg.h"
#include "odometry_msg.h"
#include "fiveai_stamped_diagnostic.h"


RosMessageParser::RosMessageParser()
{
}

void RosMessageParser::clear()
{
    _plot_map.numeric.clear();
    _registered_md5sum.clear();
    _introspection_parser.reset( new RosIntrospection::Parser );
    _builtin_parsers.clear();
    _warn_cancellation.clear();
    _warn_max_arraysize.clear();
}

double RosMessageParser::extractRealValue(
        const RosIntrospection::Variant& value,
        const std::string& item_name)
{
    if( value.getTypeID() == RosIntrospection::UINT64)
    {
        uint64_t val_i = value.extract<uint64_t>();
        double val_d = static_cast<double>(val_i);
        bool error = (val_i != static_cast<uint64_t>(val_d));
        if(error && _warnings_enabled)
        {
            _warn_cancellation.insert( item_name );
        }
        return val_d;
    }

    if( value.getTypeID() == RosIntrospection::INT64)
    {
        int64_t val_i = value.extract<int64_t>();
        double val_d = static_cast<double>(val_i);
        bool error = (val_i != static_cast<int64_t>(val_d));
        if(error && _warnings_enabled)
        {
            _warn_cancellation.insert( item_name );
        }
        return val_d;
    }

    double val_d = value.convert<double>();
    return val_d;
}



void RosMessageParser::setMaxArrayPolicy(size_t max_array_size, bool discard_entire_array)
{
    _max_array_size = max_array_size;
    _discard_large_array = discard_entire_array;
    _introspection_parser->setMaxArrayPolicy( discard_entire_array );
}

template <typename T>
bool InsertParser(RosMessageParser::ParsersMap& parsers,
                  const std::string &topic_name,
                  const std::string &md5sum)
{
    if( md5sum != T::getCompatibleKey() )
    {
        return false;
    }
    if( parsers.find(topic_name) == parsers.end())
    {
        parsers.emplace( std::piecewise_construct,
                         std::forward_as_tuple(topic_name),
                         std::forward_as_tuple(new T()) );
    }
    return true;
}

bool RosMessageParser::registerSchema(const std::string &topic_name,
                                      const std::string &md5sum,
                                      RosIntrospection::ROSType type,
                                      const std::string &definition)
{
    _registered_md5sum.insert( md5sum );

    bool inserted =
            InsertParser<GeometryMsgTwist>( _builtin_parsers, topic_name, md5sum ) ||
            InsertParser<OdometryMsgParser>( _builtin_parsers, topic_name, md5sum ) ||
            InsertParser<DiagnosticMsg>( _builtin_parsers, topic_name, md5sum ) ||
            InsertParser<FiveAiDiagnosticMsg>( _builtin_parsers, topic_name, md5sum ) ||
            InsertParser<PalStatisticsNamesParser>( _builtin_parsers, topic_name, md5sum ) ||
            InsertParser<PalStatisticsValuesParser>( _builtin_parsers, topic_name, md5sum );

    if( !inserted ) {
        _introspection_parser->registerMessageDefinition(topic_name, type, definition);
    }
    return inserted;
}

void RosMessageParser::pushMessageRef(const std::string &topic_name,
                                      const MessageRef &msg,
                                      double timestamp)
{
    auto builtin_it = _builtin_parsers.find( topic_name );
    if( builtin_it != _builtin_parsers.end() )
    {
        builtin_it->second->setUseHeaderStamp(_use_header_stamp); // bug fix #202
        builtin_it->second->pushMessageRef( builtin_it->first, msg, timestamp );
        return;
    }

    using namespace RosIntrospection;

    FlatMessage flat_container;
    RenamedValues renamed_values;

    bool max_size_ok = _introspection_parser->deserializeIntoFlatContainer(
                topic_name,
                {const_cast<uint8_t*>(msg.data()), static_cast<long>(msg.size())},
                &flat_container,
                _max_array_size );

    if( !max_size_ok && _warnings_enabled )
    {
        _warn_max_arraysize.insert(topic_name);
    }

    _introspection_parser->applyNameTransform( topic_name,
                                               flat_container,
                                               &renamed_values );
    if(_use_header_stamp)
    {
        for (const auto& it: flat_container.value)
        {
            if( it.second.getTypeID() != RosIntrospection::TIME)
            {
                continue;
            }
            const RosIntrospection::StringTreeNode* leaf1 = it.first.node_ptr;
            const RosIntrospection::StringTreeNode* leaf2 = leaf1->parent();
            if( leaf2 && leaf2->value() == "header" && leaf1->value() == "stamp")
            {
                double heder_stamp = it.second.convert<double>();

                if( heder_stamp > 0 ) {
                    timestamp = heder_stamp;
                }
                break;
            }
        }
    }

    //----------------------------
    // the KeyValue message is pretty common in ROS.
    // http://docs.ros.org/melodic/api/diagnostic_msgs/html/msg/KeyValue.html
    // Try to convert value to double
//    for( size_t n = 0; n+1 < flat_container.name.size(); n++ )
//    {
//        auto key_ptr = flat_container.name[n].first.node_ptr;
//        auto val_ptr = flat_container.name[n+1].first.node_ptr;

//        if( key_ptr->parent() == val_ptr->parent() &&
//            key_ptr->parent()->value() == "#" &&
//            key_ptr->value() == "key" &&
//            val_ptr->value() == "value" )
//        {
//            const std::string str_value = flat_container.name[n+1].second;
//            const char *start_ptr = str_value.data();
//            double num = 0;
//            auto res = absl::from_chars (start_ptr, start_ptr + str_value.size(), num);
//            if( start_ptr == res.ptr ) continue;

//            RosIntrospection::StringTreeLeaf temp_leaf;
//            temp_leaf.node_ptr = key_ptr->parent()->parent();
//            if( !temp_leaf.node_ptr ) continue;
//            temp_leaf.node_ptr = temp_leaf.node_ptr->parent();
//            if( !temp_leaf.node_ptr ) continue;

//            renamed_values.push_back( { StrCat( temp_leaf.toStdString(), "/", flat_container.name[n].second), num } );
//        }
//    }

    //----------------------------

    for(const auto& it: renamed_values )
    {
        const auto& field_name = it.first;

        const RosIntrospection::Variant& value = it.second;

        auto plot_pair = _plot_map.numeric.find( field_name );
        if( (plot_pair == _plot_map.numeric.end()) )
        {
            plot_pair = _plot_map.addNumeric( field_name );
        }

        PlotData& plot_data = plot_pair->second;
        size_t data_size = plot_data.size();

        try {
            double val_d = extractRealValue(value , field_name);
            if( !std::isnan(val_d) && !std::isinf(val_d) )
            {
                plot_data.pushBack( PlotData::Point(timestamp, val_d) );
            }
        } catch (...) {}
    }
}

void RosMessageParser::showWarnings()
{
    if( !_warn_max_arraysize.empty() )
    {
        QString message = QString("The following topics contain arrays with more than %1 elements.\n").arg(_max_array_size);
        if( _discard_large_array )
        {
            message += QString("The fields containing the extra large arrays have been discarded\n");
        }
        else{
            message += QString("These arrays were trunkated to the maximum size %1\n").arg(_max_array_size);
        }
        DialogWithItemList::warning( message, _warn_max_arraysize );
    }

    if( !_warn_cancellation.empty() )
    {
        QString message = "During the parsing process, one or more conversions to double failed"
                          " because of numerical cancellation.\n"
                          "This happens when the absolute value of a long integer exceed 2^52.\n\n"
                          "You have been warned... don't trust the following timeseries\n";
        DialogWithItemList::warning( message, _warn_cancellation );
    }
}

void RosMessageParser::extractData(PlotDataMapRef &destination, const std::string &prefix)
{
    for (auto& it: _plot_map.numeric)
    {
        appendData( destination, prefix + it.first, it.second );
    }
    _plot_map.numeric.clear();

    for (auto& it: _builtin_parsers)
    {
        it.second->extractData( destination, prefix + it.first );
    }
}


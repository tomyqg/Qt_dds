#include "ooodds.h"

#include <algorithm>
#include <iostream>

#include <QString>
#include <QObject>
#include "meter_relay.hpp"
#include <dds/pub/ddspub.hpp>
#include <dds/sub/ddssub.hpp>
#include <dds/core/ddscore.hpp>
#include <dds/core/QosProvider.hpp>
#include <rti/util/util.hpp> // for sleep()


oooDDS::oooDDS(int domain, DeviceData *Data, bool pub_sub, bool choice):
    running(0),
    pub_sub(pub_sub),
    relay_meter(choice),
    Data(Data),
    dds_participant(new dds::domain::DomainParticipant(domain)),
    dds_relaytopic(new dds::topic::Topic<two::Relay>(*this->dds_participant, "Example two_Relay")),
    dds_metertopic(new dds::topic::Topic<two::Meter>(*this->dds_participant, "Example two_Meter")),
    dds_publisher(new dds::pub::Publisher(*this->dds_participant)),
    dds_relaywriter(new dds::pub::DataWriter<two::Relay>(*this->dds_publisher, *this->dds_relaytopic)),
    dds_relaysample(new two::Relay),
    dds_meterwriter(new dds::pub::DataWriter<two::Meter>(*this->dds_publisher, *this->dds_metertopic)),
    dds_metersample(new two::Meter),
    dds_subscriber(new dds::sub::Subscriber(*this->dds_participant)),
    dds_relayreader(new dds::sub::DataReader<two::Relay> (*this->dds_subscriber, *this->dds_relaytopic)),
    dds_meterreader(new dds::sub::DataReader<two::Meter> (*this->dds_subscriber, *this->dds_metertopic))
{
    std::cout << "DDS Start in Meter!!!!" << std::endl;
    this->count = 20;
    this->set_always(false);
    this->set_delay(1000);
}

void oooDDS::dds_write()
{ 
//    if (participant == NULL) {
//        std::cout << "NULL NULL NULL" << std::endl;
//        // ... error
//    };
    // Set the second bit to 1
    this->running |= 0x10;
    this->Data->id = 0;

    while(this->count){
        if (Data->id == 1){
            this->dds_relaysample->id(this->Data->id);
            this->dds_relaysample->status(this->Data->status);
            this->dds_relaywriter->write(*this->dds_relaysample);
            emit response_pub_sub("Relay is Publishing");
            std::cout << this->Data->id<<std::endl;
            std::cout << this->Data->status<<std::endl;
            msleep(this->delaytime);
        }
        else{
            this->dds_metersample->id(this->Data->id);
            this->dds_metersample->voltage(this->Data->voltage);
            this->dds_metersample->current(this->Data->current);
            this->dds_metersample->power(this->Data->power);
            this->dds_metersample->frequency(this->Data->frequency);
            this->dds_metersample->pf(this->Data->pf);
            this->dds_meterwriter->write(*this->dds_metersample);
            emit response_pub_sub(tr("Meter is Publishing"));
            std::cout << "meter_pub~~~~"<<std::endl;
            std::cout << "ID is "<< this->Data->id<<std::endl;
            std::cout << "Voltage is " << this->Data->voltage<< " V" << std::endl;
            std::cout << "Current is " << this->Data->current<< " A" << std::endl;
            std::cout << "Power is " << this->Data->power<< " W" << std::endl;
            std::cout << "Frequency is " << this->Data->frequency<< " HZ" << std::endl;
            std::cout << "PF is " << this->Data->pf<<std::endl;
            std::cout << std::endl;
            msleep(this->delaytime);
        }
        msleep(this->delaytime);
        this->count -= !this->always;
    }
    std::cout << "FINISH" <<std::endl;
    //emit response_pub_sub("NO PUB");
    // Set the second bit to 0
    this->running &= (this->running ^ 0x10);
}

void oooDDS::dds_read_relay()
{
    emit response_pub_sub("SUBSCRIBE RELAY");
    // Set the first bit to 1
    this->running |= 0x01;

    dds::sub::DataReader<two::Relay> &reader_1 = *this->dds_relayreader;
    DeviceData &data_1 = *this->Data;
    bool &always_1 = this->always;

    // Create a ReadCondition for any data on this reader and associate a handler
    int count1 = 0;
    dds::sub::cond::ReadCondition read_condition(
        reader_1,
        dds::sub::status::DataState::any(),
        [&reader_1, &count1, &data_1, &always_1]()
    {
        // Take all samples
        dds::sub::LoanedSamples<two::Relay> samples = reader_1.take();
        for (auto sample : samples){
            if (sample.info().valid()){
                count1 += !always_1;
                data_1.id = sample.data().id();
                data_1.status = sample.data().status();
                std::cout << "sub_relay "<<std::endl;
                std::cout << "ID is "<< data_1.id <<std::endl;
                std::cout << "Status is "<< data_1.status <<std::endl;
                std::cout << std::endl;
            }
        }
    } // The LoanedSamples destructor returns the loan
    );

    // Create a WaitSet and attach the ReadCondition
    dds::core::cond::WaitSet waitset;
    waitset += read_condition;

    while (count1 < this->count && this->count != 0) {
        // Dispatch will call the handlers associated to the WaitSet conditions
        // when they activate
//        std::cout << "send relay response" << std::endl;
        emit response_on_sub(data_1.id);
        if (data_1.status == "On")
            emit response_pub_sub("On");
        else
            emit response_pub_sub("Off");
        waitset.dispatch(dds::core::Duration(this->delaytime)); // Wait up to 4s each time
    }

    waitset.detach_condition(read_condition);
    read_condition.close();
    std::cout << "send relay response" << std::endl;

    //emit response_pub_sub("NO RELAY SUB");
    // Set the first bit to 0
    this->running &= (this->running ^ 0x01);
}

void oooDDS::dds_read_meter()
{
    emit response_pub_sub("SUBSCRIBE METER");
    // Set the first bit to 1
    this->running |= 0x01;

    dds::sub::DataReader<two::Meter> &reader_1 = *this->dds_meterreader;
    DeviceData &data_1 = *this->Data;
    bool &always_1 = this->always;

    // Create a ReadCondition for any data on this reader and associate a handler
    int count1 = 0;
    dds::sub::cond::ReadCondition read_condition(
        reader_1,
        dds::sub::status::DataState::any(),
        [&reader_1, &count1, &data_1, &always_1]()
    {
        // Take all samples
        dds::sub::LoanedSamples<two::Meter> samples = reader_1.take();
        for (auto sample : samples){
            if (sample.info().valid()){
                count1 += !always_1;
                data_1.id = sample.data().id();
                data_1.voltage = sample.data().voltage();
                data_1.current = sample.data().current();
                data_1.power = sample.data().power();
                data_1.frequency = sample.data().frequency();
                data_1.pf = sample.data().pf();
            }
        }
    } // The LoanedSamples destructor returns the loan
    );

    // Create a WaitSet and attach the ReadCondition
    dds::core::cond::WaitSet waitset;
    waitset += read_condition;

    while (count1 < this->count && this->count != 0) {
        // Dispatch will call the handlers associated to the WaitSet conditions
        // when they activate
//        std::cout << "send METER response" << std::endl;
        emit response_on_sub(data_1.id);
        emit response_on_sub((double)data_1.voltage);
        emit response_on_sub((double)data_1.current);
        emit response_on_sub((double)data_1.power);
        emit response_on_sub((double)data_1.frequency);
        emit response_on_sub((double)data_1.pf);
        waitset.dispatch(dds::core::Duration(this->delaytime)); // Wait up to 4s each time
    }

    waitset.detach_condition(read_condition);
    read_condition.close();
    std::cout << "send meter response" << std::endl;

    //emit response_pub_sub("NO METER SUB");
    // Set the first bit to 0
    this->running &= (this->running ^ 0x01);
}
/*
ReaderListener *listener = new ReaderListener();
DDSDataReader *reader = subscriber-><em>create_datareader</em>(topic,
                                                               DDS_DATAREADER_QOS_DEFAULT,
                                                               listener,
                                                               <em>DDS_LIVELINESS_CHANGED_STATUS</em> |
                                                               <em>DDS_DATA_AVAILABLE_STATUS</em>);                                                              */

void oooDDS::dds_destroy()
{
    set_count(0);
    set_delay(0);
    /*
    // stop publisher
    while(this->running & 0x10){
        set_count(0);
        set_delay(0);
    }

    // stop subscriber
    while(this->running & 0x01){
        set_count(0);
        set_delay(0);
    }

    delete this->dds_participant;
    delete this->dds_relaytopic;
    delete this->dds_metertopic;
    */
}
#ifndef STREAMMAPPING_H
#define STREAMMAPPING_H

#include <QWidget>
#include "common.h"
class QRadioButton;

class StreamMapping : public QWidget
{
    Q_OBJECT
public:
    explicit StreamMapping( std::vector< std::pair<Remote_Interface,Remote_Interface_Type>>,MapSelectionEventConsumer*,void (MapSelectionEventConsumer::*)(bool), QWidget *parent = 0);
    Stream_Mapping get_stream_mapping();

signals:

public slots:
    void mapping_selection_changed(bool);
private:
    std::vector<QRadioButton*> left_selection;
    std::vector<QRadioButton*> right_selection;
    std::vector< std::pair<Remote_Interface,Remote_Interface_Type>> remote_interfaces;
    MapSelectionEventConsumer* clbk_target;
    void (MapSelectionEventConsumer::*clbk)(bool);
};

#endif // STREAMMAPPING_H

# Project Structure and UML

<aside>
<img src="https://www.notion.so/icons/pencil_purple.svg" alt="https://www.notion.so/icons/pencil_purple.svg" width="40px" />

Author: Jasmine Kumar, Xavier Lusetti

</aside>

<aside>
<img src="https://www.notion.so/icons/alert_orange.svg" alt="https://www.notion.so/icons/alert_orange.svg" width="40px" />

</aside>

# Initial Class Structure

```cpp
YTBLEND/
├── AlgorithmLayer/           
│   ├── BasicBlendAlgorithm
│   ├── IBlendAlgorithm
│   ├── RandomBlendAlgorithm
│   ├── WeightedBlendAlgorithm
├── AppInfrastructure/           
│   ├── CsvParser
│   ├── CsvSource
│   ├── DataExtractor
│   ├── DataSaver
│   ├── FileBin
│   ├── FileFormatter
│   ├── FileSource
│   ├── File_ID
│   ├── HtmlParser
│   ├── HtmlSource
│   ├── Parser
│   ├── WatchLaterParser
├── ModelLayer/                 //Model logic
│   ├── User.h/.cpp
│   ├── YouTubeData.h/.cpp
│   ├── Blend.h/.cpp
│   ├── BlendAlgorithm.h/.cpp  //Single algorithm class to start
│   ├── DataManager.h/.cpp     //Handles file parsing andstorage
│   ├── Channel
│   ├── ChatRoom
│   ├── Friend
│   ├── Message
│   ├── Video
│   └── VideoEntry  
├── ServiceLayer/           
│   ├── DataManager
│   ├── SqliteDataManager
│   ├── StorageManager
│   ├── YouTubeAPIService
├── UILayer/           
│   ├── MainFrame.h/.cpp       //Main window
│   ├── LoginPanel.h/.cpp
│   ├── BlendPanel.h/.cpp      //Video feed display
│   ├── SettingsPanel.h/.cpp
│   ├── BlendChatPanel
│   ├── BlendCreationPanel
│   ├── BlendFeedPanel
│   ├── ConfirmationDialog
│   ├── DataInstructionsPanel
│   ├── TopBar
│   ├── UIColors
│   ├── UIPages
│   ├── UserPanel
│   └── VideoCard  
└── AppLayer/                  //Glue between interface and business logic
    ├── AppController.h/.cpp   //Coordinates everything
    ├── AppState.h/.cpp        //Current app state (user, blend, etc.)
    └── EventRouter.h/.cpp     //Simple event system
```

# UML Class Diagram (core functionality only)

![YT-BLND-UML.drawio.svg](Docs/Resources/YT-BLND-uml-diag.drawio.svg)
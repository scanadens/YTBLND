# Project Structure and UML

<aside>
<img src="https://www.notion.so/icons/pencil_purple.svg" alt="https://www.notion.so/icons/pencil_purple.svg" width="40px" />

Author: Jasmine Kumar, Xavier Lusetti

</aside>

<aside>
<img src="https://www.notion.so/icons/alert_orange.svg" alt="https://www.notion.so/icons/alert_orange.svg" width="40px" />

</aside>

# Class Structure

```cpp
YTBLEND/
├── AlgorithmLayer/           
│   ├── BasicBlendAlgorithm
│   ├── IBlendAlgorithm
│   ├── RandomBlendAlgorithm
│   └── WeightedBlendAlgorithm
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
│   ├── WatchHistoryParser
│   └── YouTubeDataImporter  
├── ModelLayer/                 //Model logic
│   ├── User
│   ├── YouTubeData
│   ├── Blend
│   ├── BlendAlgorithm          //Single algorithm class to start
│   ├── DataManager             //Handles file parsing andstorage
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
│   └── YouTubeAPIService
├── UILayer/           
│   ├── MainFrame               //Main window
│   ├── LoginPanel
│   ├── BlendChatPanel          //Video feed display
│   ├── SettingsPanel
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
└── AppLayer/                   //Glue between interface and business logic
    ├── AppController           //Coordinates everything
    ├── AppState                //Current app state (user, blend, etc.)
    └── EventRouter             //Simple event system
```

# UML Class Diagram

![YT-BLND-UML.drawio.svg](Docs/Resources/YT-BLND-uml-diag.drawio.svg)
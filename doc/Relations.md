## Relations

* HAS 1:N = CHILD-OF -1 : 1:N
  - Use-case: Component HAS Interface
  - Defines the SURFACE/INTERACTION points of a component
* PART_OF_COMPOSITION : N:1 <--> COMPOSED-OF
  - Use-case: Component Instance PART_OF_COMPOSITION Component Model
  - Defines the INNER parts (hidden from the outside) of a component model
  - This enables the composition of 'higher'-level components from 'simpler' components
* (assembly) COMPOSED_OF : 1:N
  - Inverse of PART_OF_COMPOSITION
  - this name has been selected to distinguish clearly between a container (contains) and an assembly
* SUBCLASS_OF  N:1
  - Enables subclassing
  - Use case: component-model SUBCLASS_OF component_model
* INSTANCE-OF N:1
  - Use-Case: Separates a CLASS from an INSTANCE. E.g. a component instance from its model
  - Defines the model a component instance has to conform to
* (interface) CONNECTED-TO N:M
  - Defines connection between two interfaces, special information that there exists a data flow / mechanical connection between two connections
  - Together with component instances and their interfaces, this relation defines the INNER structure of a component model.
* ALIAS-OF 1:1
  - Use-case: An interface of a component MODEL can be an ALIAS-OF an INNER interface of a subcomponent.
  - Defines interfaces into the HIDDEN INNER structure of a 'higher'-level component.
* (container) IN N:M <--> CONTAINS
  - Defines: A simple contains relation for ExecutionSamples that belong to a cluster
  - Use-case:
  - * [ ] Rename to CONTAINED-BY
* PROVIDES_FEATURE_SPACE 1:N (FEAT->CLUSTER)
  - Defines: The connection of a FeatureSpace to a Cluster, which defines the Feature Space in which the clustering takes place. The edge-label holds information about the metric.
  - Use-case: 1 cluster has only one feature
  - * [ ] rename to HOSTS (or use a subrelation: ) cluster exists-in feature-space, feature space hosts cluster
* GENERATES (PART_OF - > EXEC-SAMPLE)
  - Defines: A link from an ExecutionSample to the RobotAPI that generated it. The edgelabel holds information about the parameters that need to be used in the RobotAPI to generate exactly this execution sample
  - Use-case:
  - * [ ] rename to HAS (or use a subrelation)
* NEEDS_VARIABLE_FEATURE N:M (CC->FeatureSpace)
  - Defines: The FeatureSpace in which variable input of a CognitiveCore is defined, that the CC needs to execute
  - Use-case:
      reach = CC(variable_feature_space=['start_state', 'end_state'])
      reach.execute(start_state=[0.5,0.5,0.74], end_state=[0.1,-0.23,1.4])
  - * [ ] rename DEPENDS-ON
* CONSTRAINED_BY_FEATURE N:M (CC->FeatureSpace)
  - Defines: The FeatureSpace in which a CognitiveCore is constrained
  - Use-case:
     reach = CC(constraining_feature_space=['directness'], constraining_cluster_labels=['high', 'very_high'])
  - * [ ] rename CONSTRAINED-BY
* OUTPUT_IN  N:M (RobotAPI->FeatureSpace)
  - Defines: The output Feature Space of the RobotAPI. Maybe this is overkill for the moment, since for now it is exclusively "Joint angles". But could get important when dimension 1-3 is joint angles, dimension 4 to 20 is camera pixels, dimension 21 to 40 is force-torque sensor readings
  - Use-case:
  - * [ ] changed into a property of the Robot API --> StateSpace including StateSpaceDescriptor  
* BELONGS_TO N:1 (RobotAPI->System)
  - Defines: The connection between a RobotAPI and the System to which it belongs. The idea is that there might be several APIs for the same System. Don't know  whether this is the case though.
  - Use-case:
* Robot State Space should be defined, ideally, we can reuse State Space Information between related robots, e.g., when a robot receives a hardware extension the new state space is a superset of the initial one
  * * [ ] structure the state space for reuse
  * while this task can be relevant, e.g., for capability transfer, we delay the structuring of the state space to a later iteration

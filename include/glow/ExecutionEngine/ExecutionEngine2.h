/**
 * Copyright (c) 2017-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef GLOW_EXECUTIONENGINE_EXECUTIONENGINE2_H
#define GLOW_EXECUTIONENGINE_EXECUTIONENGINE2_H

#include "glow/Backend/Backend.h"
#include "glow/Backend/CompiledFunction.h"
#include "glow/Backends/DeviceManager.h"
#include "glow/Base/Train.h"
#include "glow/Base/Traits.h"
#include "glow/Graph/Graph.h"
#include "glow/Graph/PlaceholderBindings.h"
#include "glow/Runtime/HostManager/HostManager.h"

#include "llvm/ADT/ArrayRef.h"

#include <memory>
#include <unordered_map>

namespace glow {

/// This is the ExecutionEngine. It owns the Graph, the backend, and the
/// compiled function.  The Graph, etc in this class are defined as pointers, in
/// order to erase the type and prevent the internal types from leaking out to
/// the users of this class.
class ExecutionEngine2 final {
  /// Module containing the function and supporting information.
  std::unique_ptr<Module> module_;

  /// Raw pointer to module_ this is to support module access after the module
  /// has been added to hostManager_.
  Module *rawModule_;

  /// Name of the backend being used for compilation and execution.
  std::string backendName_ = "";

  /// The HostManager for executing the compiled functions.
  std::unique_ptr<runtime::HostManager> hostManager_;

  /// Glow functions compiled for this ExecutionEngine's backend.
  std::set<std::string> compiledFunctions_;

  /// Single execution of the given \compiledFunction with the given context
  /// \bindings.
  void runInternal(ExecutionContext &context, llvm::StringRef name);

public:
  ExecutionEngine2(llvm::StringRef backend = "Interpreter");

  ~ExecutionEngine2();

  /// Set the code generator to \p backend. New code will be generated
  /// using this backend. This clears all previously loaded functions and resets
  /// the Module.
  void setBackendName(llvm::StringRef backend);

  /// Get a pointer to the backend.
  llvm::StringRef getBackendName() const;

  /// \returns the internal graph.
  Module &getModule() { return *rawModule_; }

  /// Clears the ExecutionEngine and all CompiledFunctions.
  void clear();

  /// \returns the DAG for the specified \p network.
  llvm::Expected<runtime::DAG &> getDAG(llvm::StringRef network) {
    return hostManager_->getNetworkDAG(network);
  }

  /// Compiles all functions in the Module with the given \p cctx.  This method
  /// should be invoked before the run method and can only be called once
  /// without resetting the backend.
  void compile(CompilationContext &cctx);

  /// A convenience function for the most common type of compile.
  void compile(CompilationMode mode);

  /// Context aware single execution of a function. If more than one
  /// function has been compiled by this ExecutionEngine then a name must be
  /// supplied to specify which function to run.
  void run(ExecutionContext &context);

  /// Context aware single execution of a function with the given \p
  /// name.
  void run(ExecutionContext &context, llvm::StringRef name);

  /// Context aware single execution of a function. If more than one
  /// function has been compiled by this ExecutionEngine then a name must be
  /// supplied to specify which function to run.
  void run(PlaceholderBindings &bindings);

  /// Context aware single execution of a function with the given \p
  /// name.
  void run(PlaceholderBindings &bindings, llvm::StringRef name);
};

//===----------------------------------------------------------------------===//
//         Helper methods for running the execution engine.
//===----------------------------------------------------------------------===//

/// This method updates the placeholders in \p ph with the tensor content
/// values \p inputs, in \p bindings.
void updateInputPlaceholders2(PlaceholderBindings &bindings,
                              llvm::ArrayRef<Placeholder *> ph,
                              llvm::ArrayRef<Tensor *> inputs);

/// This method updates the placeholders in the module. The placeholders are
/// found by name
///  in \p ph with the tensor content values \p inputs.
void updateInputPlaceholdersByName2(PlaceholderBindings &bindings, Module *mod,
                                    llvm::ArrayRef<llvm::StringRef> ph,
                                    llvm::ArrayRef<Tensor *> inputs);

/// Runs \p iterations iterations of the compiled function. The method updates a
/// global counter and future invocations of this method continue running
/// iterations of the batch at the next available slice.
///
/// The method updates the placeholder in \p ph with the tensors \p inputs. The
/// shape of the slice has to be identical to the shape of slices in the batch.
/// All dimensions, except for the first (batch) dimension must be identical.
///
/// The variable \p sampleCounter is consumed and updated by the function. This
/// variable records the number of samples that were consumed by the network in
/// previous iterations. The next input to be loaded is
/// (sampleCounter % batchsize).
void runBatch2(ExecutionEngine2 &EE, PlaceholderBindings &bindings,
               size_t iterations, size_t &sampleCounter,
               llvm::ArrayRef<Placeholder *> ph,
               llvm::ArrayRef<Tensor *> inputs, llvm::StringRef name = "");

} // namespace glow

#endif // GLOW_EXECUTIONENGINE_EXECUTIONENGINE2_H
